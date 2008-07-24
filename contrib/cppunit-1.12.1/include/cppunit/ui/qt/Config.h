#ifndef CPPUNIT_QTUI_CONFIG_H
#define CPPUNIT_QTUI_CONFIG_H

/*! Macro to export symbol to DLL with VC++.
 *
 * - QTTESTRUNNER_DLL_BUILD must be defined when building the DLL.
 * - QTTESTRUNNER_DLL must be defined if linking against the DLL.
 * - If none of the above are defined then you are building or linking against
 *   the static library.
 */

#if defined( QTTESTRUNNER_DLL_BUILD )
#  define QTTESTRUNNER_API __declspec(dllexport)
#elif defined ( QTTESTRUNNER_DLL )
#  define QTTESTRUNNER_API __declspec(dllimport)
#else
#  define QTTESTRUNNER_API
#endif


#endif // CPPUNIT_QTUI_CONFIG_H
