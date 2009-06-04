/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/


package assimp;

import java.util.Vector;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStream;

/**
 * Default implementation of a logger. When writing to the log,
 * jASSIMP uses the <code>Logger</code> instance returned by
 * <code>DefaultLogger.get()</code>
 *
 * @author Aramis (Alexander Gessler)
 * @version 1.0
 */
public class DefaultLogger implements Logger {


    /**
     * Helper class to combine a log stream with an error severity
     */
    private static class LogStreamInfo {
        public LogStream stream;
        public int severity;

        public LogStreamInfo(LogStream stream, int severity) {
            this.stream = stream;
            this.severity = severity;
        }
    }

    /**
     * NULL logger class. Does nothing ...
     */
    private static class NullLogger implements Logger {


        public void debug(String message) {
        } // nothing to do here ...


        public void error(String message) {
        } // nothing to do here ...


        public void warn(String message) {
        } // nothing to do here ...


        public void info(String message) {
        } // nothing to do here ...


        public void attachStream(LogStream stream, int severity) {
        } // nothing to do here ...


        public void detachStream(LogStream stream, int severity) {
        } // nothing to do here ...
    }


    /**
     * Implementation of LogStream that can be used to use a
     * <code>java.io.OutputStream</code> object directly as log stream.
     */
    public static class StreamWrapper implements LogStream {

        private OutputStream stream;

        /**
         * Construction from an existing <code>java.io.OutputStream</code> object
         *
         * @param stream May not be null
         */
        public StreamWrapper(OutputStream stream) {
            assert(null != stream);
            this.stream = stream;
        }

        public void write(String message) {
            try {
                stream.write(message.getBytes(), 0, message.length());
            } catch (IOException e) {
                // .... should't care
            }
        }

        public OutputStream getStream() {
            return stream;
        }
    }

    /**
     * Implementation of LogStream that can be used to use a
     * <code>java.io.FileWriter</code> object directly as log stream.
     */
    public static class FileStreamWrapper implements LogStream {

        private FileWriter stream;

        /**
         * Construction from an existing <code>java.io.FileWriter</code> object
         *
         * @param stream May not be null
         */
        public FileStreamWrapper(FileWriter stream) {
            assert(null != stream);
            this.stream = stream;
        }

        public void write(String message) {
            try {
                stream.write(message);
            } catch (IOException e) {
                // .... should't care
            }
        }

        public FileWriter getStream() {
            return stream;
        }
    }


    /**
     * Normal granularity of logging
     */
    public static final int LOGSEVERITY_NORMAL = 0x0;

    /**
     * Debug info will be logged, too
     */
    public static final int LOGSEVERITY_VERBOSE = 0x1;

    /**
     * Default logger. It does nothing and is used if the
     * application hasn't allocated a default logger
     */
    private static NullLogger s_nullLogger = new NullLogger();

    /**
     * The logger that is used by ASSIMP for logging
     */
    private static Logger s_logger = s_nullLogger;

    /**
     * List of logstreams to output to
     */
    private Vector<LogStreamInfo> m_avStreams;

    /**
     * One of the LOGSEVERITY_XXX constants.
     */
    @SuppressWarnings("unused")
	private int m_iLogSeverity = LOGSEVERITY_NORMAL;


    private DefaultLogger() {
    }


    /**
     * Create the default logger
     *
     * @param file    Output log file. If != <code>null</code> this will
     *                automatically add a file log stream to the logger
     * @param bErrOut If this is true an additional logstream which
     *                outputs all log messages via <code>System.err.println()</code>
     *                will be added to the logger.
     */
    public static void create(String file, boolean bErrOut) throws IOException {

        s_logger = new DefaultLogger();

        if (null != file) {
            FileWriter stream = new FileWriter(file);
            s_logger.attachStream(new FileStreamWrapper(stream), 0);
        }
        if (bErrOut) {
            s_logger.attachStream(new StreamWrapper(System.err), 0);
        }
    }

    /**
     * Create the default logger, no default log streams will be
     * attached to it.
     */
    public static void create() throws IOException {
        create(null, false);
    }

    /**
     * Supply your own implementation of <code>Logger</code> to the
     * logging system. Use this if you want to override the default
     * formatting behavior of <code>DefaultLogger</code>. You can
     * access your logger as normal, via <code>get()</code>.
     *
     * @param logger
     */
    public static void set(Logger logger) {
        s_logger = logger;
    }

    /**
     * Kill the logger ... a null logger will be used instead
     */
    public static void kill() {
        s_logger = s_nullLogger;
    }

    /**
     * Get access to the Singleton instance of the logger. This will
     * never be null. If no logger has been explicitly created via
     * <code>create()</code> this is a <code>NULLLogger</code> instance.
     * Use <code>isNullLogger()</code> to check whether the returned logger
     * is a null logger.
     *
     * @return Never null ...
     */
    public static Logger get() {
        return s_logger;
    }


    /**
     * Check whether the current logger is a null logger, which
     * doesn't log anything. Use <code>create()</code> or <code>set()</code>
     * to setup another logger.
     *
     * @return true if the curent logger is a null logger (true by default)
     */
    public static boolean isNullLogger() {
        return (s_logger instanceof NullLogger);
    }


    /**
     * Write a debug message to the log
     *
     * @param message Message to be logged
     */
    public void debug(String message) {
        this.writeToStreams("Debug:" + message + "\n", ERRORSEVERITY_DEBUGGING);
    }

    /**
     * Write an error message to the log
     *
     * @param message Message to be logged
     */
    public void error(String message) {
        this.writeToStreams("Debug:" + message + "\n", ERRORSEVERITY_ERR);
    }

    /**
     * Write a warning message to the log
     *
     * @param message Message to be logged
     */
    public void warn(String message) {
        this.writeToStreams("Debug:" + message + "\n", ERRORSEVERITY_WARN);
    }

    /**
     * Write an information message to the log
     *
     * @param message Message to be logged
     */
    public void info(String message) {
        this.writeToStreams("Debug:" + message + "\n", ERRORSEVERITY_INFO);
    }

    /**
     * Attach a log stream to the logger
     *
     * @param stream   Log stream instance
     * @param severity Error severity. Bitwise combination of the
     *                 ERRORSEVERITY_XXX constants. Specify 0 to attach the
     *                 stream to all types of log messages.
     */
    public void attachStream(LogStream stream, int severity) {

        if (0 == severity) {
            severity = ERRORSEVERITY_DEBUGGING | ERRORSEVERITY_WARN |
                    ERRORSEVERITY_ERR | ERRORSEVERITY_INFO;
        }

        for (LogStreamInfo info : this.m_avStreams) {

            if (info.stream != stream) continue;
            info.severity |= severity;
            severity = 0xcdcdcdcd;
        }
        if (0xcdcdcdcd != severity)
            this.m_avStreams.add(new LogStreamInfo(stream, severity));
    }

    /**
     * Detach a log stream from the logger
     *
     * @param stream   Log stream instance
     * @param severity Error severities to detach the stream from.
     *                 Bitwise combination of the ERRORSEVERITY_XXX constants.
     *                 Specify 0 to detach the stream from all types of log messages.
     */
    public void detachStream(LogStream stream, int severity) {

        if (0 == severity) {
            severity = ERRORSEVERITY_DEBUGGING | ERRORSEVERITY_WARN |
                    ERRORSEVERITY_ERR | ERRORSEVERITY_INFO;
        }

        for (LogStreamInfo info : this.m_avStreams) {

            if (info.stream != stream) continue;

            if (0 != (severity & ERRORSEVERITY_DEBUGGING)) {
                info.severity &= (~ERRORSEVERITY_DEBUGGING);
            }
            if (0 != (severity & ERRORSEVERITY_ERR)) {
                info.severity &= (~ERRORSEVERITY_ERR);
            }
            if (0 != (severity & ERRORSEVERITY_INFO)) {
                info.severity &= (~ERRORSEVERITY_INFO);
            }
            if (0 != (severity & ERRORSEVERITY_WARN)) {
                info.severity &= (~ERRORSEVERITY_WARN);
            }
            if (0 == info.severity) {
                this.m_avStreams.remove(info);
            }
            break;
        }
    }

    private void writeToStreams(String message, int severity) {

        for (LogStreamInfo info : this.m_avStreams) {

            if (0 == (info.severity & severity)) continue;

            info.stream.write(message);
        }
    }

    // Helpers to make access to the logging system easier for native code
    public static void _NativeCallWriteError(String message) {
        DefaultLogger.get().error(message);
    }

    public static void _NativeCallWriteWarn(String message) {
        DefaultLogger.get().warn(message);
    }

    public static void _NativeCallWriteInfo(String message) {
        DefaultLogger.get().info(message);
    }

    public static void _NativeCallWriteDebug(String message) {
        DefaultLogger.get().debug(message);
    }

}
