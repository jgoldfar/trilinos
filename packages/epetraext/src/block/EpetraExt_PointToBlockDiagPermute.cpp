#include "EpetraExt_PointToBlockDiagPermute.h"
#include "EpetraExt_BlockDiagMatrix.h"
#include "Epetra_MultiVector.h"
#include "Epetra_Import.h"
#include "Epetra_Export.h"
#include "Epetra_Comm.h"

#include <fstream>

#define MAX(x,y) ((x)>(y)?(x):(y))

//=========================================================================
EpetraExt_PointToBlockDiagPermute::EpetraExt_PointToBlockDiagPermute(const Epetra_CrsMatrix& MAT)
  :Epetra_DistObject(MAT.RowMap()),
   Matrix_(&MAT),
   PurelyLocalMode_(true),
   NumBlocks_(0),
   Blockstart_(0),
   Blockids_(0),
   BDMap_(0),
   CompatibleMap_(0),
   BDMat_(0),
   Importer_(0),
   Exporter_(0),
   ImportVector_(0),
   ExportVector_(0)
{

}
  
//=========================================================================  
// Destructor
EpetraExt_PointToBlockDiagPermute::~EpetraExt_PointToBlockDiagPermute()
{
  if(BDMap_) delete BDMap_;
  if(CompatibleMap_) delete CompatibleMap_;
  if(BDMat_) delete BDMat_;
  if(Importer_) delete Importer_;
  if(Exporter_) delete Exporter_;
  if(ImportVector_) delete ImportVector_;
  if(ExportVector_) delete ExportVector_;
}



//=========================================================================  
// Set list
int EpetraExt_PointToBlockDiagPermute::SetParameters(Teuchos::ParameterList & List){
  List_=List;

  // Local vs. global ids & mode
  NumBlocks_=List_.get("number of local blocks",0);
  if(NumBlocks_ <= 0) EPETRA_CHK_ERR(-1);
  
  Blockstart_=List_.get("block start index",(int*)0);
  if(!NumBlocks_) EPETRA_CHK_ERR(-2);
  
  Blockids_=List_.get("block entry lids",(int*)0);
  if(Blockids_)
    PurelyLocalMode_=true;
  else{
    Blockids_=List_.get("block entry gids",(int*)0);
    PurelyLocalMode_=false;
  }
  if(!Blockids_) EPETRA_CHK_ERR(-3);
  
  return 0;
}


//=========================================================================  
// Extracts the block-diagonal, builds maps, etc.
int EpetraExt_PointToBlockDiagPermute::Compute(){
  int rv=ExtractBlockDiagonal();
  //NTS: Do I need to do anything else???
  return rv;
}

//=========================================================================  
// Returns the result of a Epetra_Operator applied to a Epetra_MultiVector X in Y.
int EpetraExt_PointToBlockDiagPermute::Apply(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const{
  return -1;

}

//=========================================================================  
// Returns the result of a Epetra_Operator inverse applied to an Epetra_MultiVector X in Y.
int EpetraExt_PointToBlockDiagPermute::ApplyInverse(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const{
  // Stuff borrowed from Epetra_CrsMatrix
  int NumVectors = X.NumVectors();
  if (NumVectors!=Y.NumVectors()) {
    EPETRA_CHK_ERR(-2); // Need same number of vectors in each MV
  }

  const Epetra_MultiVector *Xp=&X;
  Epetra_MultiVector *Yp=&Y;

  // Allocate temp workspace if X==Y and there are no imports or exports
  Epetra_MultiVector * Xcopy = 0;
  if (&X==&Y && Importer_==0 && Exporter_==0) {
    Xcopy = new Epetra_MultiVector(X);
    Xp=Xcopy;
  }
  
  UpdateImportVector(NumVectors); // Make sure Import and Export Vectors are compatible
  UpdateExportVector(NumVectors);

  // If we have a non-trivial importer, we must import elements that are permuted or are on other processors
  if (Importer_){
    EPETRA_CHK_ERR(ImportVector_->Import(X, *Importer_, Insert));
    Xp=ImportVector_;
  }
  
  // If we have a non-trivial exporter, we must export elements that are permuted or belong to other processors
  if (Exporter_) {
    Yp=ExportVector_;
  }
  
  // Do the matvec 
  BDMat_->ApplyInverse(*Xp,*Yp);

  // Export if needed
  if (Exporter_) {
    Y.PutScalar(0.0);  // Make sure target is zero
    Y.Export(*ExportVector_, *Exporter_, Add); // Fill Y with Values from export vector
  }
  
  // Cleanup
  if(Xcopy) {
    delete Xcopy;
    EPETRA_CHK_ERR(1); // Return positive code to alert the user about needing extra copy of X
    return 1;
  }

  return 0;
}

//=========================================================================  
// Print method
void EpetraExt_PointToBlockDiagPermute::Print(ostream& os) const{
  if(Importer_) cout<<*Importer_<<endl;
  if(Exporter_) cout<<*Exporter_<<endl;
  if(BDMat_) cout<<*BDMat_<<endl;
}

//=========================================================================  
// Pulls the block diagonal of the matrix and then builds the BDMat_
int EpetraExt_PointToBlockDiagPermute::ExtractBlockDiagonal(){  
  int i,j;
  std::vector<int> ExtRows;
  int* LocalColIDS=0;
  int ExtSize=0,ExtCols=0,MainCols=0;
  int Nrows=Matrix_->NumMyRows();
  int *l2b,*block_offset,*l2blockid;
  const Epetra_Map &RowMap=Matrix_->RowMap();
  int *bsize=new int[NumBlocks_]; 
  int index,col,row_in_block,col_in_block,length,*colind;
  double *values;

  // Compute block size lists
  for(i=0;i<NumBlocks_;i++) 
    bsize[i]=Blockstart_[i+1]-Blockstart_[i];
  
  // Use the ScanSum function to compute a prefix sum of the number of points
  int MyMinGID;
  Matrix_->Comm().ScanSum(&NumBlocks_,&MyMinGID, 1);
  MyMinGID-=NumBlocks_;
  int *MyBlockGIDs=new int[NumBlocks_];
  for(i=0;i<NumBlocks_;i++)
    MyBlockGIDs[i]=MyMinGID+i;

  BDMap_=new Epetra_BlockMap(-1,NumBlocks_,MyBlockGIDs,bsize,0,Matrix_->Comm());
  
  // Allocate the Epetra_DistBlockMatrix
  BDMat_=new EpetraExt_BlockDiagMatrix(*BDMap_,true);  
  
  // First check to see if we can switch back to PurelyLocalMode_ if we're not
  // in it
  if(!PurelyLocalMode_){
    // Find the non-local row IDs
    ExtSize=MAX(0,Blockstart_[NumBlocks_]-RowMap.NumMyElements());// estimate
    ExtRows.reserve(ExtSize);
    
    for(i=0;i<Blockstart_[NumBlocks_];i++){
      if(RowMap.LID(Blockids_[i])==-1)
        ExtRows.push_back(Blockids_[i]);
    }

    // Switch to PurelyLocalMode_ if we never need GIDs
    int size_sum;
    ExtSize=ExtRows.size();   
    RowMap.Comm().SumAll(&ExtSize,&size_sum,1);
    if(size_sum==0){
      if(!Matrix_->Comm().MyPID()) printf("Switching to purely local mode\n");
      PurelyLocalMode_=true;
      for(i=0;i<Blockstart_[NumBlocks_];i++){
        Blockids_[i]=RowMap.LID(Blockids_[i]);
      }     
    }
  }
  
  if(PurelyLocalMode_){
    /*******************************************************/
    // Allocations
    l2b=new int[Nrows];
    block_offset=new int[Nrows];

    // Build the local-id-to-block-id list, block offset list    
    for(i=0;i<NumBlocks_;i++) {
      for(j=Blockstart_[i];j<Blockstart_[i+1];j++){
        block_offset[Blockids_[j]]=j-Blockstart_[i];
        l2b[Blockids_[j]]=i;
      }
    }
    
    // Copy the data to the EpetraExt_BlockDiagMatrix        
    for(i=0;i<Nrows;i++) {
      int block_num = l2b[i];
      if(block_num>=0 && block_num<NumBlocks_) {
        row_in_block=block_offset[i];
        Matrix_->ExtractMyRowView(i,length,values,colind);
        int Nnz=0;
        for (j = 0; j < length; j++) {
          col = colind[j];
          if(col < Nrows && l2b[col]==block_num){
            Nnz++;
            col_in_block = block_offset[col];
            index = col_in_block * bsize[block_num] + row_in_block;
            (*BDMat_)[block_num][index]=values[j];
          }
        }
        /* Handle the case of a zero row. */
        /* By just putting a 1 on the diagonal. */
        if (Nnz == 0) {
          index = row_in_block * bsize[block_num] + row_in_block;
          (*BDMat_)[block_num][index]=1.0;
        }
      }
    }

    // Build the compatible map for import/export
    l2blockid=new int[Nrows];
    for(i=0;i<Nrows;i++) 
      l2blockid[Blockstart_[l2b[i]]+block_offset[i]]=Matrix_->RowMap().GID(i);

    // Build the Compatible Map, Import/Export Objects
    CompatibleMap_=new Epetra_Map(-1,Nrows,l2blockid,0,Matrix_->Comm());
    
  }
  else{
    /*******************************************************/      
    // Do the import to grab matrix entries
    // Allocate temporaries for import
    Epetra_Map TmpMap(-1,ExtSize, &ExtRows[0],0,Matrix_->Comm());; 
    Epetra_CrsMatrix TmpMatrix(Copy,TmpMap,0);
    Epetra_Import TmpImporter(TmpMap,RowMap);

    TmpMatrix.Import(*Matrix_,TmpImporter,Insert);
    TmpMatrix.FillComplete();
      
    ExtCols=TmpMatrix.NumMyCols();
    MainCols=Matrix_->NumMyCols();
        

    // Build the column reidex - main matrix
    LocalColIDS=new int[MainCols+ExtCols];      
    for(i=0;i<MainCols;i++){
      int GID=Matrix_->GCID(i);
      int MainLID=RowMap.LID(GID);
      if(MainLID!=-1) LocalColIDS[i]=MainLID;
      else{
        int ExtLID=TmpMatrix.LRID(GID);
        if(ExtLID!=-1) LocalColIDS[i]=Nrows+ExtLID;
        else LocalColIDS[i]=-1;
      }
    }

    // Build the column reidex - ext matrix
    for(i=0;i<ExtCols;i++){
      int GID=TmpMatrix.GCID(i);
      int MainLID=RowMap.LID(GID);
      if(MainLID!=-1) LocalColIDS[MainCols+i]=MainLID;
      else{
        int ExtLID=TmpMatrix.LRID(GID);          
        if(ExtLID!=-1) LocalColIDS[MainCols+i]=Nrows+ExtLID;
        else LocalColIDS[MainCols+i]=-1;
      }
    }

    // Allocations
    l2b=new int[Nrows+ExtSize];
    block_offset=new int[Nrows+ExtSize];  
  
    // Build l2b/block_offset with the expanded local index
    //NTS: Uses the same ordering of operation as the above routine, which is why
    //it works.
    for(i=0;i<Nrows+ExtSize;i++) block_offset[i]=l2b[i]=-1;    
    int ext_idx=0;
    for(i=0;i<NumBlocks_;i++) {
      for(j=Blockstart_[i];j<Blockstart_[i+1];j++){
        int LID=RowMap.LID(Blockids_[j]);
        if(LID==-1) {LID=Nrows+ext_idx;ext_idx++;}
        block_offset[LID]=j-Blockstart_[i];
        l2b[LID]=i;          
      }
    }
    
    // Copy the data to the EpetraExt_BlockDiagMatrix from Matrix_
    for(i=0;i<Nrows;i++) {
      int block_num = l2b[i];
      if(block_num>=0 && block_num<NumBlocks_) {
        row_in_block=block_offset[i];
        Matrix_->ExtractMyRowView(i,length,values,colind);
        int Nnz=0;
        for (j = 0; j < length; j++) {
          col = LocalColIDS[colind[j]];
          if(col!=-1 && l2b[col]==block_num){
            Nnz++;
            col_in_block = block_offset[col];
            index = col_in_block * bsize[block_num] + row_in_block;
            (*BDMat_)[block_num][index]=values[j];
          }
        }
        /* Handle the case of a zero row. */
        /* By just putting a 1 on the diagonal. */
        if (Nnz == 0) {
          index = row_in_block * bsize[block_num] + row_in_block;
          (*BDMat_)[block_num][index]=values[j];
        }
      }
    }
    
    // Copy the data to the EpetraExt_BlockDiagMarix from TmpMatrix
    for(i=0;i<ExtSize;i++) {
      int block_num = l2b[Nrows+i];
      if(block_num>=0 && block_num<NumBlocks_) {
        row_in_block=block_offset[Nrows+i];
        TmpMatrix.ExtractMyRowView(i,length,values,colind);
        int Nnz=0;
        for (j = 0; j < length; j++) {
          col = LocalColIDS[MainCols+colind[j]];
          if(col!=-1 && l2b[col]==block_num){
            Nnz++;
            col_in_block = block_offset[col];
            index = col_in_block * bsize[block_num] + row_in_block;
            (*BDMat_)[block_num][index]=values[j];
          }
        }
        /* Handle the case of a zero row. */
        /* By just putting a 1 on the diagonal. */
        if (Nnz == 0) {
          index = row_in_block * bsize[block_num] + row_in_block;
          (*BDMat_)[block_num][index]=1.0;
        }
      }      
    }
    
    // Build the compatible map for import/export
    l2blockid=new int[Blockstart_[NumBlocks_]];
    for(i=0;i<Nrows;i++){
      int bkid=l2b[i];
      if(bkid>-1) l2blockid[Blockstart_[bkid]+block_offset[i]]=RowMap.GID(i);
    }
    // NTS: This is easier - if we imported it, we know we need it
    for(i=0;i<ExtSize;i++)
      l2blockid[Blockstart_[l2b[Nrows+i]]+block_offset[Nrows+i]]=TmpMatrix.GRID(i);
    
    // Build the Compatible Map, Import/Export Objects
    CompatibleMap_=new Epetra_Map(-1,Blockstart_[NumBlocks_],l2blockid,0,Matrix_->Comm());
        
  }//end else

  // Set BDMat_'s parameter list and compute!
  Teuchos::ParameterList dummy,inList;
  inList=List_.get("blockdiagmatrix: list",dummy);
  BDMat_->SetParameters(inList);  
  BDMat_->Compute();
  
  // Build importer/exporter
  if(!CompatibleMap_->SameAs(Matrix_->DomainMap()))
    Importer_ = new Epetra_Import(*CompatibleMap_,Matrix_->DomainMap());
  if(!CompatibleMap_->SameAs(Matrix_->RangeMap()))
    Exporter_ = new Epetra_Export(*CompatibleMap_,Matrix_->RangeMap());
  
  // Cleanup
  delete [] LocalColIDS;
  delete [] block_offset;
  delete [] l2b;
  delete [] l2blockid;
  delete [] bsize;
  delete [] MyBlockGIDs;

  return 0;
}



//=======================================================================================================
void EpetraExt_PointToBlockDiagPermute::UpdateImportVector(int NumVectors) const {    
  if(Importer_ != 0) {
    if(ImportVector_ != 0) {
      if(ImportVector_->NumVectors() != NumVectors) { 
	delete ImportVector_; 
	ImportVector_= 0;
      }
    }
    if(ImportVector_ == 0) 
      ImportVector_ = new Epetra_MultiVector(*CompatibleMap_,NumVectors); // Create import vector if needed
  }
  return;
}
//=======================================================================================================
void EpetraExt_PointToBlockDiagPermute::UpdateExportVector(int NumVectors) const {    
  if(Exporter_ != 0) {
    if(ExportVector_ != 0) {
      if(ExportVector_->NumVectors() != NumVectors) { 
	delete ExportVector_; 
	ExportVector_= 0;
      }
    }
    if(ExportVector_ == 0) 
      ExportVector_ = new Epetra_MultiVector(*CompatibleMap_,NumVectors); // Create Export vector if needed
  }
  return;
}




//=========================================================================  
int EpetraExt_PointToBlockDiagPermute::Import(const Epetra_SrcDistObject& A, const Epetra_Import& Importer, Epetra_CombineMode CombineMode, const Epetra_OffsetIndex *Indexor){
 return -1;
}

//=========================================================================  
int EpetraExt_PointToBlockDiagPermute::Import(const Epetra_SrcDistObject& A, const Epetra_Export& Exporter, Epetra_CombineMode CombineMode, const Epetra_OffsetIndex *Indexor){
 return -1;
}

//=========================================================================  
int EpetraExt_PointToBlockDiagPermute::Export(const Epetra_SrcDistObject& A, const Epetra_Import & Importer, Epetra_CombineMode CombineMode, const Epetra_OffsetIndex *Indexor){
 return -1;
}

//=========================================================================  
int EpetraExt_PointToBlockDiagPermute::Export(const Epetra_SrcDistObject& A, const Epetra_Export& Exporter, Epetra_CombineMode CombineMode, const Epetra_OffsetIndex *Indexor){
 return -1;
}

//=========================================================================  
// Allows the source and target (\e this) objects to be compared for compatibility, return nonzero if not.
int EpetraExt_PointToBlockDiagPermute::CheckSizes(const Epetra_SrcDistObject& Source){
  return -1;
}

//=========================================================================  
// Perform ID copies and permutations that are on processor.
int EpetraExt_PointToBlockDiagPermute::CopyAndPermute(const Epetra_SrcDistObject& Source,
                   int NumSameIDs, 
                   int NumPermuteIDs,
                   int * PermuteToLIDs,
                   int * PermuteFromLIDs,
                   const Epetra_OffsetIndex * Indexor){
  return -1;
}

//=========================================================================  
// Perform any packing or preparation required for call to DoTransfer().
int EpetraExt_PointToBlockDiagPermute::PackAndPrepare(const Epetra_SrcDistObject& Source,
                   int NumExportIDs,
                   int* ExportLIDs,
                   int& LenExports,
                   char*& Exports,
                   int& SizeOfPacket,
                   int* Sizes,
                   bool & VarSizes,
                   Epetra_Distributor& Distor){
  return -1;
}

//=========================================================================  
// Perform any unpacking and combining after call to DoTransfer().
int EpetraExt_PointToBlockDiagPermute::UnpackAndCombine(const Epetra_SrcDistObject& Source, 
                     int NumImportIDs,
                     int* ImportLIDs, 
                     int LenImports,
                     char* Imports,
                     int& SizeOfPacket, 
                     Epetra_Distributor& Distor,
                     Epetra_CombineMode CombineMode,
                     const Epetra_OffsetIndex * Indexor){
  return -1;
}


