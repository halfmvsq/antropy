#include "AntropyApp.h"
#include "common/InputParser.h"
#include "logic/app/Logging.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>


int main( int argc, char* argv[] )
{
    auto logFailure = []() {
        spdlog::debug( "------------------------ END SESSION (FAILURE) ------------------------" );
    };

    try
    {
        Logging logging;

        spdlog::debug( "------------------------ BEGIN SESSION ------------------------" );
        AntropyApp::logPreamble();

        InputParams params;

        if ( EXIT_FAILURE == parseCommandLine( argc, argv, params ) )
        {
            logFailure();
            return EXIT_FAILURE;
        }

        if ( ! params.set )
        {
            spdlog::debug( "Command line arguments not specified" );
            logFailure();
            return EXIT_FAILURE;
        }

        logging.setConsoleSinkLevel( params.consoleLogLevel );
        spdlog::debug( "Parsed command line parameters:\n{}", params );

        // Create, initialize, and run the application:
        AntropyApp app;
        app.loadImagesFromParams( params );
        app.init();
        app.run();
    }
    catch ( const std::runtime_error& e )
    {
        spdlog::critical( "Runtime error: {}", e.what() );
        logFailure();
        return EXIT_FAILURE;
    }
    catch ( const std::exception& e )
    {
        spdlog::critical( "Exception: {}", e.what() );
        logFailure();
        return EXIT_FAILURE;
    }
    catch ( ... )
    {
        spdlog::critical( "Unknown exception" );
        logFailure();
        return EXIT_FAILURE;
    }

    spdlog::debug( "------------------------ END SESSION (SUCCESS) ------------------------" );
    return EXIT_SUCCESS;
}
