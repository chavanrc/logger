#pragma once

#include "Logger.hpp"

namespace findoc {
    /**
     * Reads the contents of a file into a string.
     *
     * @param filename The name of the file to read.
     *
     * @returns The contents of the file as a string.
     */
    template<class CharContainer>
    static size_t fileGetContents(const char *filename, CharContainer *v) {
        ::FILE *fp = ::fopen(filename, "rb");
        if (!fp) {
            auto error = fmt::format("{} could not open file", filename);
            RAISE(Exception, error);
        }
        ::fseek(fp, 0, SEEK_END);
        long sz = ::ftell(fp);
        v->resize(static_cast<typename CharContainer::size_type>(sz));
        if (sz) {
            ::rewind(fp);
            size_t ret = ::fread(&(*v)[0], 1, v->size(), fp);
            C4_CHECK(ret == (size_t) sz);
        }
        ::fclose(fp);
        return v->size();
    }

    /**
     * Reads the contents of a file into a container.
     *
     * @param filename The name of the file to read.
     * @param cc The container to read the file into.
     *
     * @returns None
     */
    template<class CharContainer>
    static CharContainer fileGetContents(const char *filename) {
        CharContainer cc;
        fileGetContents(filename, &cc);
        return cc;
    }

    /**
     * Initializes the logger.
     *
     * @param configFile The path to the logger configuration file.
     *
     * @returns None
     */
    static void initLogger(const std::string &configFile = "") {
        if (!configFile.empty()) {
            auto contents = findoc::fileGetContents<std::string>(configFile.c_str());
            ryml::Tree tree = ryml::parse_in_arena(ryml::to_csubstr(contents));
            findoc::LoggerConfig loggerConfig(tree);
            findoc::Logger::init(loggerConfig);
        } else {
            findoc::Logger::init();
        }
    }
}  // namespace findoc
