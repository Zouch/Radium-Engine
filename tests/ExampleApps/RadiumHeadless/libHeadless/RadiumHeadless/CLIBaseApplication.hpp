#pragma once
#include <RadiumHeadless/CLI/CLI.hpp>
#include <RadiumHeadless/Headless.hpp>

/**
 * Base class for command line interface Radium application
 *
 */
class HEADLESS_API CLIBaseApplication
{

  protected:
    /// The data file to manage
    std::string m_dataFile = {""};

    /** Command-line parameters parser.
     * All derived applications will share parameters --help and --file <scene_file>.
     * Derived classes should extend this parser to add their own parameters
     */
    CLI::App cmdline_parser;

  public:
    /// Base constructor.
    CLIBaseApplication() {
        cmdline_parser.add_option( "-f,--file", m_dataFile, "Data file to process." )
            ->check( CLI::ExistingFile ); // add ->required() to force user to give a filename;
    };
    /// Base destructor
    virtual ~CLIBaseApplication() = default;

    /// adapter allowing to add a command line option on an application the same way than using
    /// CLI11 directly.
    /// \see https://cliutils.github.io/CLI11/book/chapters/options.html
    template <typename... Args>
    CLI::Option* add_option( Args&&... args ) {
        return cmdline_parser.add_option( args... );
    }

    /// adapter allowing to add a command line flag on an application the same way than using CLI11
    /// directly.
    /// \see https://cliutils.github.io/CLI11/book/chapters/flags.html
    template <typename... Args>
    CLI::Option* add_flag( Args&&... args ) {
        return cmdline_parser.add_flag( args... );
    }

    /**
     * Application initialization method.
     *
     * This method is called by the main function to initialize the application giving the
     * parameters from the command line.
     * @param argc number of parameter
     * @param argv array of string representing the parameters
     * @return 0 if the application is correctly initialized or an application dependant error code
     * if something went wrong.
     */
    virtual int init( int argc, const char* argv[] ) {
        (void)argc;
        (void)argv;
        return 0;
    }

    /**
     * Run the application.
     *
     * @return 0 if the application was correctly ran or an application dependant error code if
     * something went wrong.
     */
    virtual int run( float timeStep = 0 ) = 0;

    /**
     * Get the filename given using the option --file <filename> or -f <filename>
     */
    inline std::string getDataFileName() const { return m_dataFile; }

    /**
     * Set the filename given using the option --file <filename> or -f <filename>
     */
    inline void setDataFileName( std::string filename ) { m_dataFile = filename; }
};
