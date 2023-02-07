#pragma once

#include <cerrno>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

/**
 * throw(Exception( "Shit happens", ERROR_TRACE);
 */

#define ERROR_TRACE __FUNCTION__, __FILE__, __LINE__

/**
 * EXCEPTION(Exception,Buddhist_Observation);
      creates an new exception class "Buddhist_Observation", derived from the base "Exception"
 */
#define EXCEPTION(Super, Current)                                                       \
    class Current : public Super {                                                      \
       public:                                                                          \
        explicit Current(std::string desc, std::string func = "?", std::string f = "?", \
                         const int32_t l = -1, std::string n = #Current)                \
            : Super(std::move(desc), std::move(func), std::move(f), l, std::move(n)) {} \
    }

/**
 * RAISE( Buddhist_Observation, "There is " << 0 << " shit that happens" );
 */
#define RAISE(Err, msg)                     \
    {                                       \
        std::ostringstream oss;             \
        oss << msg;                         \
        throw(Err(oss.str(), ERROR_TRACE)); \
    }

namespace findoc {
    class Exception : public std::exception {
    public:
        /*!
            This constructor is not supposed to be used with hand-made location arguments
            but with metadata provided by the compiler.
            Use the ERROR_TRACE macro to raise the exception, for example :
                throw( Exception( "Shit evolves", ERROR_TRACE );
        */
        Exception(std::string desc, std::string func, std::string f, const int32_t l,
                  std::string n = "Exception")
                : description_(std::move(desc)),
                  function_(std::move(func)),
                  file_(std::move(f)),
                  line_(l),
                  name_(std::move(n)) {
            message_ =
                    fmt::format("<{}> {}\t\t{} @ {}:{}", name_, description_, function_, file_, line_);
        }

        /**
         * Destructor exception class.
         */
        ~Exception() = default;

        /**
         * Returns a string describing the exception.
         *
         * @returns A string describing the exception.
         */
        [[nodiscard]] const char *what() const noexcept override { return message_.c_str(); }

    private:
        //! Description of the exception
        std::string description_;

        //! Function where the exception has been raised
        std::string function_;

        //! File where the exception has been raised
        std::string file_;

        //! Line where the exception has been raised
        int32_t line_;

        //! Name of the current exception class
        std::string name_;

        //! Assembled message
        std::string message_;
    };

    EXCEPTION(Exception, ConfigException);
}  // namespace findoc