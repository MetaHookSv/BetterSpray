#include <utf8filesystem.h>

#include <cassert>
#include <cstdio>
#include <iterator>
#include <string>

static void TestConversions()
{
    const char*    name = "\xE4\xB8\xAD\xE6\x96\x87 \xF0\x9F\x8E\xA8.png";
    const wchar_t* wide = L"\u4E2D\u6587 \U0001F3A8.png";
    assert(std::wstring(wide) == filesystem_utf8::ToWide(name));
    assert(std::string(name) == filesystem_utf8::FromWide(wide));
    assert(std::wstring(L"plain.png") == filesystem_utf8::ToWide("plain.png"));
    assert(std::string("plain.png") == filesystem_utf8::FromWide(L"plain.png"));
    assert(true == filesystem_utf8::ToWide("\xFF").empty());
    assert(true == filesystem_utf8::ToWide(nullptr).empty());
    assert(true == filesystem_utf8::FromWide(nullptr).empty());
}

static void TestFiles()
{
    wchar_t tempPath[MAX_PATH]{};
    assert(0 != GetTempPathW(MAX_PATH, tempPath));
    wchar_t tempFile[MAX_PATH]{};
    assert(0 != GetTempFileNameW(tempPath, L"bsu", 0, tempFile));
    assert(TRUE == DeleteFileW(tempFile));
    assert(TRUE == CreateDirectoryW(tempFile, nullptr));

    const std::wstring root          = tempFile;
    const std::wstring directory     = root + L"\\\u4E2D\u6587\u6587\u4EF6\u5939";
    const std::string  directoryUTF8 = filesystem_utf8::FromWide(directory.c_str());
    assert(true == filesystem_utf8::CreateFolder(directoryUTF8.c_str()));
    const auto nestedParent = directory + L"\\\u591A\u7EA7";
    const auto nested       = nestedParent + L"\\\u5B50\u76EE\u5F55";
    const auto nestedUTF8   = filesystem_utf8::FromWide(nested.c_str());
    assert(true == filesystem_utf8::CreateFolder(nestedUTF8.c_str()));
    assert(true == filesystem_utf8::CreateFolder(nestedUTF8.c_str()));
    assert(false == filesystem_utf8::CreateFolder("\xFF"));
    // This filename fits Win32's UTF-16 limit while exceeding MAX_PATH UTF-8 bytes.
    constexpr size_t   longNameLength = 100;
    const std::wstring names[]        = {L"plain.png", L"\u55B7\u6F06.png", L"\U0001F3A8.png", std::wstring(longNameLength, L'\u6D4B') + L".png"};
    for (const auto& name : names)
    {
        const auto file   = directory + L"\\" + name;
        HANDLE     handle = CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        assert(INVALID_HANDLE_VALUE != handle);
        assert(TRUE == CloseHandle(handle));
        const auto  fileUTF8       = filesystem_utf8::FromWide(file.c_str());
        const DWORD fileAttributes = filesystem_utf8::GetAttributes(fileUTF8.c_str());
        assert(INVALID_FILE_ATTRIBUTES != fileAttributes);
        assert(0 == (fileAttributes & FILE_ATTRIBUTE_DIRECTORY));
        assert(false == filesystem_utf8::CreateFolder(fileUTF8.c_str()));
    }

    const DWORD attributes = filesystem_utf8::GetAttributes(directoryUTF8.c_str());
    assert(INVALID_FILE_ATTRIBUTES != attributes);
    assert(FILE_ATTRIBUTE_DIRECTORY == (attributes & FILE_ATTRIBUTE_DIRECTORY));
    assert(INVALID_FILE_ATTRIBUTES == filesystem_utf8::GetAttributes((directoryUTF8 + "\\missing.png").c_str()));
    assert(INVALID_FILE_ATTRIBUTES == filesystem_utf8::GetAttributes("\xFF"));

    WIN32_FIND_DATAW data{};
    HANDLE           find = filesystem_utf8::FindFirst((directoryUTF8 + "\\*.png").c_str(), &data);
    assert(INVALID_HANDLE_VALUE != find);
    bool found[std::size(names)]{};
    do
    {
        const auto nameUTF8 = filesystem_utf8::FromWide(data.cFileName);
        const auto name     = filesystem_utf8::ToWide(nameUTF8.c_str());
        for (size_t i = 0; i < std::size(names); ++i)
            if (names[i] == name)
                found[i] = true;
    } while (FindNextFileW(find, &data));
    assert(ERROR_NO_MORE_FILES == GetLastError());
    assert(TRUE == FindClose(find));
    for (bool value : found)
        assert(true == value);

    find = filesystem_utf8::FindFirst((directoryUTF8 + "\\*.jpg").c_str(), &data);
    assert(INVALID_HANDLE_VALUE == find);
    assert(INVALID_HANDLE_VALUE == filesystem_utf8::FindFirst("\xFF", &data));

    const auto previous = filesystem_utf8::ToWide(filesystem_utf8::GetWorkingDirectory().c_str());
    assert(TRUE == SetCurrentDirectoryW(directory.c_str()));
    assert(directoryUTF8 == filesystem_utf8::GetWorkingDirectory());
    assert(TRUE == SetCurrentDirectoryW(previous.c_str()));

    for (const auto& name : names)
        assert(TRUE == DeleteFileW((directory + L"\\" + name).c_str()));
    assert(TRUE == RemoveDirectoryW(nested.c_str()));
    assert(TRUE == RemoveDirectoryW(nestedParent.c_str()));
    assert(TRUE == RemoveDirectoryW(directory.c_str()));
    assert(TRUE == RemoveDirectoryW(root.c_str()));
}

int main(int argc, char** argv)
{
    const auto codePage = GetACP();
    std::printf("Process ACP: %u\n", codePage);
    std::fflush(stdout);
    if (argc == 2 && static_cast<UINT>(std::stoul(argv[1])) != codePage)
    {
        std::puts("Requested manifest code page is unavailable on this Windows version; skipping.");
        return 77;
    }
    TestConversions();
    TestFiles();
    std::puts("UTF-8 filenames, directory enumeration, attributes and working directory passed.");
}
