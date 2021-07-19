#include <RadiumHeadless/CLIViewer.hpp>
#include <iostream>

#include <Engine/Rendering/ForwardRenderer.hpp>

#ifdef ADD_ASSIMP_LOADER
#    include <IO/AssimpLoader/AssimpFileLoader.hpp>
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

int main( int argc, const char* argv[] ) {
    //! [Creating the viewer with custom psarameters]
    bool showWindow {false};
    bool usercontrolWindows {false};
    CLIViewer viewer;
    auto winOpt = viewer.add_flag( "-w,--window", showWindow, "Map the viewer window." );
    viewer
        .add_flag( "-i,--interactive",
                   usercontrolWindows,
                   "Wait for the user to close window before exiting." )
        ->needs( winOpt );
    //! [Creating the viewer with custom parameters]

    //! [Configuring the viewer : initialize OpenGL and the Engine]

    if ( int code = viewer.init( argc, argv ) ) { return code; }
    auto viewerParameters = viewer.getViewerParameters();
    //! [Configuring the viewer : initialize OpenGL and the Engine]

    //! [Populating the viewer with needed services]
    viewer.setRenderer( new Ra::Engine::Rendering::ForwardRenderer() );
#ifdef ADD_ASSIMP_LOADER
    viewer.addDataLoader( new Ra::IO::AssimpFileLoader() );
#endif
    //! [Populating the viewer with needed services]

    //! [Bind the OpenGLContext if needed]
    viewer.bindOpenGLContext();
    //! [Bind the OpenGLContext if needed]

    //! [Setup the Scene to render]
    if ( viewer.getDataFileName().empty() ) { viewer.setDataFileName( "Assets/astroboy.dae" ); }
    viewer.compileScene();
    //! [Setup the Scene to render]

    //! [Running the application]
    auto saveFrame = [&viewer]( const std::string& filename ) {
        size_t w, h;
        std::cout << "Writing image " << filename << std::endl;
        auto writtenPixels = viewer.grabFrame( w, h );
        stbi_write_png( filename.c_str(), w, h, 4, writtenPixels.get(), w * 4 * sizeof( uchar ) );
    };

    if ( showWindow ) { viewer.showWindow(); }

    if ( viewerParameters.m_animationEnable )
    {
        // compute 2s of animation at 60fps
        for ( int i = 0; i < 120; ++i )
        {
            viewer.oneFrame( i );

            if ( showWindow ) { viewer.swapBuffers(); }
            else
            {
                auto filename = viewerParameters.m_imgPrefix + std::to_string( i ) + ".png";
                saveFrame( filename );
            }
        }
    }
    else
    {
        // compute one picture
        viewer.oneFrame();

        if ( showWindow ) { viewer.swapBuffers(); }
        else
        {
            auto filename = viewerParameters.m_imgPrefix + ".png";
            saveFrame( filename );
        }
    }

    if ( usercontrolWindows ) { viewer.waitForClose(); }
    //! [Running the application]

    //! [Unbind the OpenGLContext when no more needed]
    viewer.bindOpenGLContext( false );
    //! [Unbind the OpenGLContext when no more needed]

    return 0;
}
