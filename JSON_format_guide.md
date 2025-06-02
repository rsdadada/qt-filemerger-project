# JSON Configuration File Format for File Merging

## 1. Function Overview

This document describes the JSON file format used to specify a list of files to be imported and selected for merging within the File Merger application. This feature allows for batch loading of files without manually selecting each one through the application's interface.

## 2. JSON File Structure

The JSON file must be a valid JSON object.

*   **Root Object:** The file consists of a single root JSON object.
*   **Core Key:** This object must contain a key named `"files_to_merge"`.
*   **Value Type:** The value associated with the `"files_to_merge"` key must be an array of strings. Each string in this array represents a path to a file that should be imported.

## 3. File Path Conventions

*   **Path Types:**
    *   **Absolute Paths:** Fully qualified paths to files are supported (e.g., `/home/user/documents/file.txt` on Linux/macOS, `C:\Users\User\Documents\file.txt` on Windows).
    *   **Relative Paths:** Paths can be relative to the directory where the JSON configuration file itself is located. For example, if your JSON file is in `/data/configs/myfiles.json` and contains the path `"input/source.log"`, the application will look for `/data/configs/input/source.log`.
*   **Path Separators:**
    *   It is highly recommended to use forward slashes (`/`) as path separators for cross-platform compatibility (e.g., `path/to/file.txt`).
    *   While Windows-style backslashes (`\`) might work if properly escaped in the JSON string (e.g., `C:\Users\User\file.txt`), using forward slashes is generally more robust.

## 4. Example JSON

Here is an example of a valid JSON configuration file:

```json
{
  "files_to_merge": [
    "/users/jane/documents/report.pdf",
    "project_data/sources/chapter1.txt",
    "../archive/old_notes.md",
    "C:\Work\Spreadsheets\financials.xlsx"
  ]
}
```

## 5. Key Considerations

*   **File Encoding:** The JSON configuration file itself should be UTF-8 encoded for maximum compatibility, especially if file paths contain non-ASCII characters.
*   **File Existence:** The application will check if each specified file exists and is readable. Files that cannot be found or accessed will be skipped. A summary of any such errors will be provided after the import attempt.
*   **JSON Validity:** Ensure the file is a well-formed JSON. You can use online validators to check your JSON structure if you encounter issues.
```
