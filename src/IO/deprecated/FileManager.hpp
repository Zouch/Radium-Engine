#pragma once

#include <fstream>
#include <string>

namespace Ra {
namespace IO {

/**
 * The class FileManager gives the base common interface for handling loading/storing data from/to
 * files.
 * \warning will be removed once integration of file saving is done in new IO, see
 * https://github.com/STORM-IRIT/Radium-Engine/issues/362
 * \todo reimplement in IO
 * \deprecated see https://github.com/STORM-IRIT/Radium-Engine/issues/362
 */
template <typename DATA, bool Binary = false>
class FileManager
{
  public:
    /// ENUM
    enum LogEntryType {
        LogEntry_Normal,
        LogEntry_Warning,
        LogEntry_Error,
    };

    /// CONSTRUCTOR
    inline FileManager() = default; // Default constructor.

    /// DESTRUCTOR
    inline virtual ~FileManager() = default; // Destructor.

    /// INTERFACE
    inline bool load( const std::string& filename,
                      DATA& data,
                      const bool SAVE_LOG_FILE =
                          false ); // Return true if the data is correctly loaded, false otherwise.
    inline bool save( const std::string& filename,
                      const DATA& data,
                      const bool SAVE_LOG_FILE =
                          false ); // Return true if the data is correctly stored, false otherwise.

    /// LOG
    inline std::string log() const; // Return the log of the last action performed.

  protected:
    /// LOG
    inline void addLogEntry( const std::string& text ); // Add a line in the log file. A newline
                                                        // character will be automatically added.
    inline void
    addLogWarningEntry( const std::string& text );           // Add a warning line in the log file.
    inline void addLogErrorEntry( const std::string& text ); // Add an error line in the log file.
    inline void addLogEntry( const std::string& text, const LogEntryType type );

    /// INTERFACE
    virtual std::string fileExtension() const = 0; // Return the extension given to the files.
    virtual bool importData( std::istream& file,
                             DATA& data ) = 0; // Load data from a given file. Return false if an
                                               // error occurs, true otherwise.
    virtual bool exportData( std::ostream& file,
                             const DATA& data ) = 0; // Store given data into a given file. Return
                                                     // false if an error occurs, true otherwise.

  private:
    /// LOG
    inline void resetLog();                             // Reset the log string.
    inline void saveLog( const std::string& filename ); // Save the log into a text file.

    /// VARIABLE
    std::string m_log { "" };
};

} // namespace IO
} // namespace Ra

#include <IO/deprecated/FileManager.inl>
