/**
//@HEADER
// ************************************************************************
//
//                   Trios: Trilinos I/O Support
//                 Copyright 2011 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//Questions? Contact Ron A. Oldfield (raoldfi@sandia.gov)
//
// *************************************************************************
//@HEADER
 */
/*
 * multicast_service_test.cpp
 *
 *  Created on: Nov 14, 2011
 *      Author: thkorde
 */


#include "Trios_config.h"
#include "Trios_nssi_client.h"
#include "Trios_nssi_xdr.h"

#include "Teuchos_CommandLineProcessor.hpp"
#include "Teuchos_StandardCatchMacros.hpp"
#include "Teuchos_oblackholestream.hpp"

#include <multicast_service_args.h>
#include "multicast_test.h"
#include "multicast_debug.h"


#include <mpi.h>
#include <assert.h>
#include <unistd.h>

#include <iostream>
#include <ostream>
#include <fstream>


// Prototypes for client and server codes
int multicast_server_main(struct multicast_args &args, MPI_Comm server_comm);
int multicast_client_main (struct multicast_args &args, nssi_service* multicast_svc, MPI_Comm client_comm);


/* -------------- private methods -------------------*/


int print_args(
        std::ostream &out,
        const struct multicast_args &args,
        const char *prefix)
{
    if (args.client_flag && args.server_flag)
        out << prefix << " ------------  ARGUMENTS (client and server) ----------- " << std::endl;
    else if (args.client_flag && !args.server_flag)
        out << prefix << " ------------  ARGUMENTS (client) ----------- " << std::endl;
    else if (!args.client_flag && args.server_flag)
        out << prefix << " ------------  ARGUMENTS (server) ----------- " << std::endl;

    out << prefix << " \tserver-url       = " << args.server_url << std::endl;

    if (args.client_flag) {
        out << prefix << " \ttransport        = " << args.transport_name << std::endl;
        out << prefix << " \tio-method        = " << args.io_method_name << std::endl;
        out << prefix << " \tnum-trials        = " << args.num_trials << std::endl;
        out << prefix << " \tnum-reqs         = " << args.num_reqs << std::endl;
        out << prefix << " \tlen              = " << args.len << std::endl;
        out << prefix << " \tvalidate         = " << ((args.validate_flag)?"true":"false") << std::endl;
        out << prefix << " \tresult-file      = " <<
                (args.result_file.empty()?"<stdout>":args.result_file) << std::endl;
        out << prefix << " \tresult-file-mode = " << args.result_file_mode << std::endl;
    }
    out << prefix << " \tdebug            = " << args.debug_level << std::endl;
    out << prefix << " \tlogfile          = " << args.logfile << std::endl;
    out << prefix << " ----------------------------------- " << std::endl;

    return 0;
}





int main(int argc, char *argv[])
{
    int np=1, rank=0;
    int splitrank, splitsize;
    int rc = 0;
    nssi_service multicast_svc[2];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);

    MPI_Barrier(MPI_COMM_WORLD);

    Teuchos::oblackholestream blackhole;
    std::ostream &out = ( rank == 0 ? std::cout : blackhole );

    struct multicast_args args;

    const int num_io_methods = 6;
    const int io_method_vals[] = {
            MULTICAST_EMPTY_REQUEST_SYNC, MULTICAST_EMPTY_REQUEST_ASYNC,
            MULTICAST_GET_SYNC,           MULTICAST_GET_ASYNC,
            MULTICAST_PUT_SYNC,           MULTICAST_PUT_ASYNC};
    const char * io_method_names[] = {
            "empty-request-sync", "empty-request-async",
            "get-sync",           "get-async",
            "put-sync",           "put-async"};

    const int num_nssi_transports = 5;
    const int nssi_transport_vals[] = {
            NSSI_RPC_PTL,
            NSSI_RPC_IB,
            NSSI_RPC_GEMINI,
            NSSI_RPC_BGPDCMF,
            NSSI_RPC_BGQPAMI,
            NSSI_RPC_MPI};
    const char * nssi_transport_names[] = {
            "ptl",
            "ib",
            "gni",
            "dcmf",
            "mpi"
    };


    // Initialize arguments
    args.transport=NSSI_DEFAULT_TRANSPORT;
    args.delay = 1;
    args.io_method = MULTICAST_EMPTY_REQUEST_SYNC;
    args.debug_level = LOG_WARN;
    args.num_trials = 1;
    args.num_reqs = 1;
    args.len = 1;
    args.result_file_mode = "a";
    args.result_file = "";
    args.url_file[0] = "";
    args.url_file[1] = "";
    args.logfile = "";
    args.client_flag = true;
    args.server_flag = true;
    args.timeout = 500;
    args.num_retries = 5;
    args.validate_flag = true;
    args.server_url[0] = "";
    args.server_url[1] = "";

    bool success = true;

    /**
     * We make extensive use of the \ref Teuchos::CommandLineProcessor for command-line
     * options to control the behavior of the test code.   To evaluate performance,
     * the "num-trials", "num-reqs", and "len" options control the amount of data transferred
     * between client and server.  The "io-method" selects the type of data transfer.  The
     * server-url specifies the URL of the server.  If running as a server, the server-url
     * provides a recommended URL when initializing the network transport.
     */
    try {

        //out << Teuchos::Teuchos_Version() << std::endl << std::endl;

        // Creating an empty command line processor looks like:
        Teuchos::CommandLineProcessor parser;
        parser.setDocString(
                "This example program demonstrates a simple data-transfer service "
                "built using the NEtwork Scalable Service Interface (Nessie)."
        );

        /* To set and option, it must be given a name and default value.  Additionally,
           each option can be given a help std::string.  Although it is not necessary, a help
           std::string aids a users comprehension of the acceptable command line arguments.
           Some examples of setting command line options are:
         */

        parser.setOption("delay", &args.delay, "time(s) for client to wait for server to start" );
        parser.setOption("timeout", &args.timeout, "time(ms) to wait for server to respond" );
        parser.setOption("server", "no-server", &args.server_flag, "Run the server" );
        parser.setOption("client", "no-client", &args.client_flag, "Run the client");
        parser.setOption("len", &args.len, "The number of structures in an input buffer");
        parser.setOption("debug",(int*)(&args.debug_level), "Debug level");
        parser.setOption("logfile", &args.logfile, "log file");
        parser.setOption("num-trials", &args.num_trials, "Number of trials (experiments)");
        parser.setOption("num-reqs", &args.num_reqs, "Number of reqs/trial");
        parser.setOption("result-file", &args.result_file, "Where to store results");
        parser.setOption("result-file-mode", &args.result_file_mode, "Write mode for the result");
        parser.setOption("server-url-1", &args.server_url[0], "URL client uses to find the server 1");
        parser.setOption("server-url-2", &args.server_url[1], "URL client uses to find the server 2");
        parser.setOption("server-url-file-1", &args.url_file[0], "File that has URL client uses to find server 1");
        parser.setOption("server-url-file-2", &args.url_file[1], "File that has URL client uses to find server 2");
        parser.setOption("validate", "no-validate", &args.validate_flag, "Validate the data");

        // Set an enumeration command line option for the io_method

        parser.setOption("io-method", &args.io_method, num_io_methods, io_method_vals, io_method_names,
                "I/O Methods for the example: \n"
                "\t\t\tempty-request-sync : Send an empty request - synchronous\n"
                "\t\t\tempty-request-async: Send an empty request - asynchronous\n"
                "\t\t\tget-sync : Servers pull data from client - synchronous\n"
                "\t\t\tget-async: Servers pull data from client - asynchronous\n"
                "\t\t\tput-sync : Servers push data from client - synchronous\n"
                "\t\t\tput-async: Servers push data from client - asynchronous"
                );

        // Set an enumeration command line option for the io_method
        parser.setOption("transport", &args.transport, num_nssi_transports, nssi_transport_vals, nssi_transport_names,
                "NSSI transports (not all are available on every platform): \n"
                "\t\t\tportals : Cray or Schutt\n"
                "\t\t\tinfiniband : libibverbs\n"
                "\t\t\tgemini : Cray\n"
                "\t\t\tmpi : isend/irecv implementation\n"
                );



        /* There are also two methods that control the behavior of the
           command line processor.  First, for the command line processor to
           allow an unrecognized a command line option to be ignored (and
           only have a warning printed), use:
         */
        parser.recogniseAllOptions(true);

        /* Second, by default, if the parser finds a command line option it
           doesn't recognize or finds the --help option, it will throw an
           std::exception.  If you want prevent a command line processor from
           throwing an std::exception (which is important in this program since
           we don't have an try/catch around this) when it encounters a
           unrecognized option or help is printed, use:
         */
        parser.throwExceptions(false);

        /* We now parse the command line where argc and argv are passed to
           the parse method.  Note that since we have turned off std::exception
           throwing above we had better grab the return argument so that
           we can see what happened and act accordingly.
         */
        Teuchos::CommandLineProcessor::EParseCommandLineReturn parseReturn= parser.parse( argc, argv );

        if( parseReturn == Teuchos::CommandLineProcessor::PARSE_HELP_PRINTED ) {
            return 0;
        }

        if( parseReturn != Teuchos::CommandLineProcessor::PARSE_SUCCESSFUL   ) {
            return 1; // Error!

        }

        // Here is where you would use these command line arguments but for this example program
        // we will just print the help message with the new values of the command-line arguments.
        //if (rank == 0)
        //    out << "\nPrinting help message with new values of command-line arguments ...\n\n";

        //parser.printHelpMessage(argv[0],out);

    }

    TEUCHOS_STANDARD_CATCH_STATEMENTS(true,std::cerr,success);

    log_debug(args.debug_level, "%d: Finished processing arguments", rank);


    if (!success) {
        MPI_Abort(MPI_COMM_WORLD, 1);
    }


    if (!args.server_flag && args.client_flag) {
        /* initialize logger */
        if (args.logfile.empty()) {
            logger_init(args.debug_level, NULL);
        } else {
            char fn[1024];
            sprintf(fn, "%s.client.%03d.log", args.logfile.c_str(), rank);
            logger_init(args.debug_level, fn);
        }
    } else if (args.server_flag && !args.client_flag) {
        /* initialize logger */
        if (args.logfile.empty()) {
            logger_init(args.debug_level, NULL);
        } else {
            char fn[1024];
            sprintf(fn, "%s.server.%03d.log", args.logfile.c_str(), rank);
            logger_init(args.debug_level, fn);
        }
    } else if (args.server_flag && args.client_flag) {
        /* initialize logger */
        if (args.logfile.empty()) {
            logger_init(args.debug_level, NULL);
        } else {
            char fn[1024];
            sprintf(fn, "%s.%03d.log", args.logfile.c_str(), rank);
            logger_init(args.debug_level, fn);
        }
    }

    log_level debug_level = args.debug_level;

    // Communicator used for both client and server (may split if using client and server)
    MPI_Comm comm;

    log_debug(debug_level, "%d: Starting multicast-service test", rank);

    /**
     * Since this test can be run as a server, client, or both, we need to play some fancy
     * MPI games to get the communicators working correctly.  If we're executing as both
     * a client and a server, we split the communicator so that the client thinks its
     * running by itself.
     */
    if (args.client_flag && args.server_flag) {
        if (np < 3) {
            log_error(debug_level, "Must use at least 3 MPI processes for client and server mode");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        // Split the communicators. Processors with color=0 are servers.

        int color = ((rank == 0)||(rank == 1)) ? 0 : 1; // two server
        MPI_Comm_split(MPI_COMM_WORLD, color, rank, &comm);

        MPI_Comm_rank(comm, &splitrank);
        MPI_Comm_size(comm, &splitsize);

        //    std::cout << "rank=" << rank << "/" << np << ", color=" << color <<
        //            ", new_rank=" << newrank << "/" << newsize << std::endl << std::endl;
        //
        //    std::cout << "my_url=" << my_url <<  ", server_url=" << args.server_url << std::endl;
    }
    else {
        MPI_Comm_dup(MPI_COMM_WORLD, &comm);
    }

    /**
     * Initialize the Nessie interface by specifying a transport, encoding scheme, and a
     * recommended URL.  \ref NSSI_DEFAULT_TRANSPORT is usually the best choice, since it
     * is often the case that only one type of transport exists on a particular platform.
     * Currently supported transports are \ref NSSI_RPC_PTL, \ref NSSI_RPC_GNI, and
     * \ref NSSI_RPC_IB.  We only support one type of encoding scheme so NSSI_DEFAULT_ENCODE
     * should always be used for the second argument.   The URL can be specified (as we did for
     * the server, or NULL (as we did for the client).  This is a recommended value.  Use the
     * \ref nssi_get_url function to find the actual value.
     */
    if (args.server_flag && !args.server_url[rank].empty()) {
        // use the server URL as suggested URL
        nssi_rpc_init((nssi_rpc_transport)args.transport, NSSI_DEFAULT_ENCODE, args.server_url[rank].c_str());
    }
    else {
        nssi_rpc_init((nssi_rpc_transport)args.transport, NSSI_DEFAULT_ENCODE, NULL);
    }

    // Get the Server URL
    std::string my_url(NSSI_URL_LEN, '\0');
    nssi_get_url((nssi_rpc_transport)args.transport, &my_url[0], NSSI_URL_LEN);

    // Broadcast the server URL to all the clients
    args.server_url[0].resize(NSSI_URL_LEN, '\0');
    args.server_url[1].resize(NSSI_URL_LEN, '\0');
    if (args.server_flag && args.client_flag) {
        args.server_url[0] = my_url;
        MPI_Bcast(&args.server_url[0][0], args.server_url[0].size(), MPI_CHAR, 0, MPI_COMM_WORLD);
        args.server_url[1] = my_url;
        MPI_Bcast(&args.server_url[1][0], args.server_url[1].size(), MPI_CHAR, 1, MPI_COMM_WORLD);
    }

    else if (!args.server_flag && args.client_flag){
        if (args.server_url[0].empty()) {

            // check to see if we're supposed to get the URL from a file
            if (!args.url_file[0].empty()) {
                // Fetch the server URL from a file
                sleep(1);
                log_debug(debug_level, "Reading from file %s", args.url_file[0].c_str());
                std::ifstream urlfile (args.url_file[0].c_str());
                if (urlfile.is_open()) {
                    if (urlfile.good())
                        getline(urlfile, args.server_url[0]);
                }
                else {
                    log_error(debug_level, "Failed to open server_url_file=%s", args.url_file[0].c_str());
                    exit(1);
                }
                urlfile.close();
                log_debug(debug_level, "URL = %s", args.server_url[0].c_str());
            }
            else {
                log_error(debug_level, "Need to set --server-url-1=[ADDR] or --server-url-file-1=[PATH]");
            }
        }
        if (args.server_url[1].empty()) {

            // check to see if we're supposed to get the URL from a file
            if (!args.url_file[1].empty()) {
                // Fetch the server URL from a file
                sleep(1);
                log_debug(debug_level, "Reading from file %s", args.url_file[1].c_str());
                std::ifstream urlfile (args.url_file[1].c_str());
                if (urlfile.is_open()) {
                    if (urlfile.good())
                        getline(urlfile, args.server_url[1]);
                }
                else {
                    log_error(debug_level, "Failed to open server_url_file=%s", args.url_file[1].c_str());
                    exit(1);
                }
                urlfile.close();
                log_debug(debug_level, "URL = %s", args.server_url[1].c_str());
            }
            else {
                log_error(debug_level, "Need to set --server-url-1=[ADDR] or --server-url-file-1=[PATH]");
            }
        }
    }

    else if (args.server_flag && !args.client_flag) {
        args.server_url[0] = my_url;
        // If the url_file value is set, write the url to a file
        if (!args.url_file[0].empty()) {
            std::ofstream urlfile (args.url_file[0].c_str());
            if (urlfile.is_open()) {
                urlfile << args.server_url[0].c_str() << std::endl;
            }
            urlfile.close();
            log_debug(debug_level, "Wrote url to file %s", args.url_file[0].c_str());
        }

        args.server_url[1] = my_url;
        // If the url_file value is set, write the url to a file
        if (!args.url_file[1].empty()) {
            std::ofstream urlfile (args.url_file[1].c_str());
            if (urlfile.is_open()) {
                urlfile << args.server_url[1].c_str() << std::endl;
            }
            urlfile.close();
            log_debug(debug_level, "Wrote url to file %s", args.url_file[1].c_str());
        }
    }



    // Set the debug level for the multicast service.
    multicast_debug_level = args.debug_level;

    // Print the arguments after they've all been set.
    args.io_method_name = io_method_names[args.io_method];
    print_args(out, args, "%");


    //------------------------------------------------------------------------------
    /** If we're running this job with a server, the server always executes on nodes 0 and 1.
     *  In this example, the server is two process.
     */
    if (args.server_flag && ((rank == 0)|(rank == 1))) {
        rc = multicast_server_main(args, comm);
        log_debug(debug_level, "Server is finished");
    }

    // ------------------------------------------------------------------------------
     /**  The parallel client will execute this branch.  The root node, nodes 0 and 1, of the client connects
      *   connects with the server, using the \ref nssi_get_service function.  Then the root
      *   broadcasts the service description to the other clients before starting the main
      *   loop of the client code by calling \ref multicast_client_main.
      */
    else {
        int i;
        int client_rank;

        // get rank within the client communicator
        MPI_Comm_rank(comm, &client_rank);

        nssi_init((nssi_rpc_transport)args.transport);

        // Only one process needs to connect to the service
        // TODO: Make get_service a collective call (some transports do not need a connection)
        //if (client_rank == 0) {
        {

            sleep(args.delay);  // give server time to get started

            // connect to remote server
            for (i=0; i < args.num_retries; i++) {
                log_debug(debug_level, "Try to connect to server: attempt #%d", i);
                rc=nssi_get_service((nssi_rpc_transport)args.transport, args.server_url[0].c_str(), args.timeout, &multicast_svc[0]);
                if (rc == NSSI_OK)
                    break;
                else if (rc != NSSI_ETIMEDOUT) {
                    log_error(multicast_debug_level, "could not get svc description: %s",
                            nssi_err_str(rc));
                    break;
                }
            }
            // connect to remote server
            for (i=0; i < args.num_retries; i++) {
                log_debug(debug_level, "Try to connect to server: attempt #%d", i);
                rc=nssi_get_service((nssi_rpc_transport)args.transport, args.server_url[1].c_str(), args.timeout, &multicast_svc[1]);
                if (rc == NSSI_OK)
                    break;
                else if (rc != NSSI_ETIMEDOUT) {
                    log_error(multicast_debug_level, "could not get svc description: %s",
                            nssi_err_str(rc));
                    break;
                }
            }
        }

        //MPI_Bcast(&rc, 1, MPI_INT, 0, comm);

        if (rc == NSSI_OK) {
            if (client_rank == 0) log_debug(debug_level, "Connected to service on attempt %d\n", i);

            // Broadcast the service description to the other clients
            //log_debug(multicast_debug_level, "Bcasting svc to other clients");
            //MPI_Bcast(&multicast_svc, sizeof(nssi_service), MPI_BYTE, 0, comm);

            log_debug(debug_level, "Starting client main");
            // Start the client code
            multicast_client_main(args, &multicast_svc[0], comm);


            MPI_Barrier(comm);

            // Tell one of the clients to kill the server
            if (client_rank == 0) {
                log_debug(debug_level, "%d: Halting multicast service", rank);
                rc = nssi_kill(&multicast_svc[0], 0, 5000);
                rc = nssi_kill(&multicast_svc[1], 0, 5000);
            }
        }

        else {
            if (client_rank == 0)
                log_error(debug_level, "Failed to connect to service after %d attempts: ABORTING", i);
            success = false;
            //MPI_Abort(MPI_COMM_WORLD, -1);
        }

        nssi_fini((nssi_rpc_transport)args.transport);

    }

    log_debug(debug_level, "%d: clean up nssi", rank);
    MPI_Barrier(MPI_COMM_WORLD);

    // Clean up nssi_rpc
    rc = nssi_rpc_fini((nssi_rpc_transport)args.transport);
    if (rc != NSSI_OK)
        log_error(debug_level, "Error in nssi_rpc_fini");

    log_debug(debug_level, "%d: MPI_Finalize()", rank);
    MPI_Finalize();

    logger_fini();

    if(success && (rc == NSSI_OK))
      out << "\nEnd Result: TEST PASSED" << std::endl;
    else
        out << "\nEnd Result: TEST FAILED" << std::endl;

    return ((success && (rc==NSSI_OK)) ? 0 : 1 );
}
