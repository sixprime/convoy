#include "convoy/commands/generate.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>

using namespace commands;

struct Manifest
{
    std::string packageName;
    std::string packageVersion;
};

static std::vector<std::filesystem::path> GetFilesToCompile(std::filesystem::path path)
{
    std::vector<std::filesystem::path> files;

    for (const auto& it: std::filesystem::recursive_directory_iterator(path))
    {
        const auto& path = it.path();
        if (path.extension() == ".cpp")
        {
            files.emplace_back(path);
        }
    }

    return files;
}

static void GenerateForNinja(std::filesystem::path path)
{
    const auto buildFile = path / "build.ninja";
    std::ofstream file { buildFile };
    if (file.is_open())
    {
        file << "# Autogenerated by convoy\n\n"
            << "ninja_required_version = 1.10\n"
            << "\n"
            << "root = .\n"
            << "builddir = build\n"
            << "cxx = clang++\n"
            << "cflags = -std=c++17 -Wall -Wextra -Wpedantic -Iinclude/ -Isrc/\n"
            << "ldflags = -L$builddir\n"
            << "\n"
            << "rule cxx\n"
            << "  command = $cxx $cflags -c $in -o $out\n"
            << "\n"
            << "rule link\n"
            << "  command = $cxx $ldflags $in -o $out\n"
            << "\n";

        std::vector<std::filesystem::path> srcFiles = GetFilesToCompile(path / "src");
        for (const std::filesystem::path& srcFile: srcFiles)
        {
            std::string srcFileRelativePathString = std::filesystem::relative(srcFile, path / "src").string();
            std::replace(srcFileRelativePathString.begin(), srcFileRelativePathString.end(), '\\', '/'); // I wish I didn't have to do that...

            std::string objFileRelativePathString = std::filesystem::relative(srcFile, path / "src").replace_extension(".o").string();
            std::replace(objFileRelativePathString.begin(), objFileRelativePathString.end(), '\\', '/');

            file << "build $builddir/" << objFileRelativePathString << ": cxx $root/src/" << srcFileRelativePathString << "\n";
        }
        file << "\n";

        file << "build " << path.stem().string();
#ifdef _WIN32
        file << ".exe";
#endif // _WIN32
        file << ": link $\n";
        for (const std::filesystem::path& srcFile: srcFiles)
        {
            std::string objFileRelativePathString = std::filesystem::relative(srcFile, path / "src").replace_extension(".o").string();
            std::replace(objFileRelativePathString.begin(), objFileRelativePathString.end(), '\\', '/');

            file << "  $builddir/" << objFileRelativePathString << " $\n";
        }
        file << "\n";

        file << "default " << path.stem().string();
#ifdef _WIN32
        file << ".exe\n";
#else
        file << "\n";
#endif // _WIN32
        file << "\n";

        file << "build all: phony " << path.stem().string();
#ifdef _WIN32
        file << ".exe\n";
#else
        file << "\n";
#endif // _WIN32

        file.close();
    }
}

void Generate::Execute(std::string buildSystem, std::filesystem::path path)
{
    if (path == ".")
    {
        path = std::filesystem::current_path();
    }

    if (buildSystem == "ninja")
    {
        GenerateForNinja(path);
    }
    else
    {
        std::cerr << "Build system generator for '" << buildSystem << "' not supported!" << std::endl;
        return;
    }
}