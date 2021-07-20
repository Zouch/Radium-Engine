#pragma once
#include <IO/deprecated/FileManager.hpp>

namespace Ra {
namespace IO {

//////////////////////////////////////////////////////////////////////////////
// INTERFACE
//////////////////////////////////////////////////////////////////////////////
template <typename DATA, bool Binary>
inline bool FileManager<DATA, Binary>::load( const std::string& filename,
                                             DATA& data,
                                             const bool SAVE_LOG_FILE ) {
    bool status = true;
    resetLog();
    addLogEntry( "Filename        : " + filename );
    addLogEntry( "Expected Format : " + fileExtension() );
    addLogEntry( "File Type       : " + std::string( Binary ? "Binary" : "Text" ) );
    addLogEntry( "Loading start..." );
    std::ifstream file(
        filename, std::ios_base::in | ( Binary ? std::ios_base::binary : std::ios_base::in ) );
    if ( !( status = file.is_open() ) ) {
        addLogWarningEntry(
            "Error occurred while opening the file. Trying to see if extension is missing..." );
        file.open( filename + "." + fileExtension(),
                   std::ios_base::in | ( Binary ? std::ios_base::binary : std::ios_base::in ) );
        if ( !( status = file.is_open() ) ) {
            addLogErrorEntry( "Error occured while opening the file. HINT: FILENAME IS WRONG." );
        }
    }
    if ( status ) {
        addLogEntry( "File opened successfully." );
        addLogEntry( "Starting to import the data." );
        status = importData( file, data );
        addLogEntry( "Import " + ( ( status ) ? std::string( "DONE." ) : std::string( "FAILED." ) ),
                     ( ( status ) ? LogEntry_Normal : LogEntry_Error ) );
        file.close();
        addLogEntry( "File closed." );
    }
    addLogEntry( "Loading " + filename + " ended." );
    if ( SAVE_LOG_FILE ) { saveLog( filename + "_load" ); }
    return status;
}

template <typename DATA, bool Binary>
inline bool FileManager<DATA, Binary>::save( const std::string& filename,
                                             const DATA& data,
                                             const bool SAVE_LOG_FILE ) {
    bool status = true;
    resetLog();
    addLogEntry( "Filename         : " + filename );
    addLogEntry( "Exporting Format : " + fileExtension() );
    addLogEntry( "File Type        : " + std::string( Binary ? "Binary" : "Text" ) );
    addLogEntry( "Saving start..." );
    std::ofstream file( filename + "." + fileExtension(),
                        std::ios_base::out | std::ios_base::trunc |
                            ( Binary ? std::ios_base::binary : std::ios_base::out ) );
    if ( !( status = file.is_open() ) ) {
        addLogErrorEntry( "Error occured while opening the file." );
    }
    if ( status ) {
        addLogEntry( "File opened successfully." );
        addLogEntry( "Starting to export the data..." );
        status = exportData( file, data );
        addLogEntry( "Export " + ( ( status ) ? std::string( "DONE." ) : std::string( "FAILED." ) ),
                     ( ( status ) ? LogEntry_Normal : LogEntry_Error ) );
        file.close();
        addLogEntry( "File closed." );
    }
    addLogEntry( "Saving " + filename + " ended." );
    if ( SAVE_LOG_FILE ) { saveLog( filename + "_save" ); }
    return status;
}

//////////////////////////////////////////////////////////////////////////////
// LOG
//////////////////////////////////////////////////////////////////////////////
template <typename DATA, bool Binary>
inline std::string FileManager<DATA, Binary>::log() const {
    return m_log;
}

template <typename DATA, bool Binary>
inline void FileManager<DATA, Binary>::addLogEntry( const std::string& text ) {
    addLogEntry( text, LogEntry_Normal );
}

template <typename DATA, bool Binary>
inline void FileManager<DATA, Binary>::addLogWarningEntry( const std::string& text ) {
    addLogEntry( text, LogEntry_Warning );
}

template <typename DATA, bool Binary>
inline void FileManager<DATA, Binary>::addLogErrorEntry( const std::string& text ) {
    addLogEntry( text, LogEntry_Error );
}

template <typename DATA, bool Binary>
inline void FileManager<DATA, Binary>::addLogEntry( const std::string& text,
                                                    const LogEntryType type ) {
    switch ( type ) {
    case LogEntry_Normal: {
        m_log += text;
    } break;

    case LogEntry_Warning: {
        m_log += "\n--- LogEntry_Warning : " + text + " ---\n";
    } break;

    case LogEntry_Error: {
        m_log += "\n### LogEntry_Error : " + text + " ###\n";
    } break;

    default:
        break;
    }

    m_log += "\n";
}

template <typename DATA, bool Binary>
inline void FileManager<DATA, Binary>::resetLog() {
    m_log = "====== FILE MANAGER LOG ======\n";
}

template <typename DATA, bool Binary>
inline void FileManager<DATA, Binary>::saveLog( const std::string& filename ) {
    std::ofstream file( filename + ".log" );
    if ( !file.is_open() ) { return; }
    file << log();
    file.close();
}

} // namespace IO
} // namespace Ra
