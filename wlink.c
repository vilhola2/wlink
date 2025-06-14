#include <stdio.h>
#include <string.h>
#include <shlwapi.h>
#include <windows.h>

int msys2_to_win(char *path, DWORD len) {
    if (len >= 3 && path[0] == '/' && path[2] == '/') {
        if (!isalpha(path[1])) return 1;
        path[0] = toupper(path[1]);
        path[1] = ':';
    }
    for (size_t i = 0; path[i]; i++) {
        if (path[i] == '/') path[i] = '\\';
    }
    return 0;
}

int win_to_msys2(char *path, DWORD len) {
    if (len >= 3 && path[1] == ':' && path[2] == '\\') {
        if (path[0] < 'A' || path[0] > 'Z') return 1;
        path[1] = tolower(path[0]);
        path[0] = '/';
    }
    for (size_t i = 0; path[i]; i++) {
        if (path[i] == '\\') path[i] = '/';
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Error: Bad number of arguments! Expected 3, got %d\n", argc - 1);
        return 1;
    }
    char *in     = NULL, *out     = NULL,
         *abs_in = NULL, *abs_out = NULL;
    int ret_val = 0;
    if (!(in = strdup(argv[1])) || !(out = strdup(argv[2]))) {
        perror("Error: Failed to duplicate string");
        free(in);
        free(out);
        ret_val = 1;
        goto cleanup;
    }
    const size_t in_len = strlen(in);
    const size_t out_len = strlen(out);
    if (msys2_to_win(in, in_len) == 1) {
        fprintf(stderr, "Error: Invalid drive letter\n");
        ret_val = 1;
        goto cleanup;
    }
    msys2_to_win(out, out_len);
    DWORD in_attrs = GetFileAttributesA(in);
    if (in_attrs == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "Failed to get file attributes: %lu\n", GetLastError());
        ret_val = 1;
        goto cleanup;
    }
    DWORD out_attrs = GetFileAttributesA(out);
    if (out_attrs == INVALID_FILE_ATTRIBUTES && GetLastError() != 2) {
        fprintf(stderr, "Failed to get file attributes: %lu\n", GetLastError());
        ret_val = 1;
        goto cleanup;
    }
    DWORD abs_in_len  = GetFullPathNameA(in,  0, NULL, NULL);
    DWORD abs_out_len = GetFullPathNameA(out, 0, NULL, NULL);
    if (!(abs_in = malloc(abs_in_len)) || !(abs_out = malloc(abs_out_len))) {
        fprintf(stderr, "Error: Malloc failed\n");
        ret_val = 1;
        goto cleanup;
    }
    if (!GetFullPathNameA(in, abs_in_len, abs_in, NULL) ||
        !GetFullPathNameA(out, abs_out_len, abs_out, NULL)) {
        fprintf(stderr, "Error: Failed to get full path names. Error code: %lu\n", GetLastError());
        ret_val = 1;
        goto cleanup;
    }
    if (!(in_attrs & FILE_ATTRIBUTE_DIRECTORY) &&
        out_attrs & FILE_ATTRIBUTE_DIRECTORY) {
        char *basename = PathFindFileNameA(in);
        size_t len = abs_out_len + 1 + strlen(basename) + 1; // to be safe
        char *temp = realloc(abs_out, len);
        if (!temp) {
            fprintf(stderr, "Error: Malloc failed\n");
            ret_val = 1;
            goto cleanup;
        }
        abs_out = temp;
        PathAppendA(abs_out, basename);
    }
    if (PathFileExistsA(abs_out)) {
        fprintf(stderr, "Error: Output file exists\n");
        ret_val = 1;
        goto cleanup;
    }
    printf("in: %s, out: %s\n", abs_in, abs_out);
    if (!CreateSymbolicLinkA(abs_out, abs_in, (in_attrs & FILE_ATTRIBUTE_DIRECTORY) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0 )) { // 0 == file
        fprintf(stderr, "Error: Failed to create symbolic link. Error code: %lu\n", GetLastError());
        ret_val = 1;
        goto cleanup;
    }
    win_to_msys2(abs_in, abs_in_len);
    win_to_msys2(abs_out, abs_out_len);
    printf("Created symbolic link: %s -> %s\n", abs_out, abs_in);
cleanup:
    free(in);
    free(out);
    free(abs_in);
    free(abs_out);
    return ret_val;
}
