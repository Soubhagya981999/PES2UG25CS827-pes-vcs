Customizable line height
The default line height has been increased for improved accessibility. You can choose to enable a more compact line height from the view settings menu.

‎.gitignore‎
Original file line number	Diff line number	Diff line change
@@ -5,6 +5,7 @@ test_tree

# Object files
*.o
*.obsidian

# PES repository (created during testing)
.pes/
@@ -15,3 +16,5 @@ test_tree
*~
.vscode/
.idea/
‎README.md‎


Original file line number	Diff line number	Diff line change
@@ -46,21 +46,21 @@ If unset, it defaults to `"PES User <pes@localhost>"`.

### File Inventory

| File             | Role                                  | Your Task                                   |
| ---------------- | ------------------------------------- | ------------------------------------------- |
| `pes.h`          | Core data structures and constants    | Do not modify                               |
| `object.c`       | Content-addressable object store      | Implement `object_write`, `object_read`     |
| `tree.h`         | Tree object interface                 | Do not modify                               |
| `tree.c`         | Tree serialization and construction   | Implement `tree_parse`, `tree_serialize`, `tree_from_index` |
| `index.h`        | Staging area interface                | Do not modify                               |
| `index.c`        | Staging area (text-based index file)  | Implement `index_load`, `index_save`, `index_add`, `index_status` |
| `commit.h`       | Commit object interface               | Do not modify                               |
| `commit.c`       | Commit creation and history           | Implement `head_read`, `head_update`, `commit_create` |
| `pes.c`          | CLI entry point and command dispatch  | Implement `cmd_commit`                      |
| `test_objects.c`  | Phase 1 test program                  | Do not modify                               |
| `test_tree.c`     | Phase 2 test program                  | Do not modify                               |
| `test_sequence.sh`| End-to-end integration test           | Do not modify                               |
| `Makefile`        | Build system                          | Do not modify                               |
| File               | Role                                 | Your Task                                          |
| ------------------ | ------------------------------------ | -------------------------------------------------- |
| `pes.h`            | Core data structures and constants   | Do not modify                                      |
| `object.c`         | Content-addressable object store     | Implement `object_write`, `object_read`            |
| `tree.h`           | Tree object interface                | Do not modify                                      |
| `tree.c`           | Tree serialization and construction  | Implement `tree_from_index`                        |
| `index.h`          | Staging area interface               | Do not modify                                      |
| `index.c`          | Staging area (text-based index file) | Implement `index_load`, `index_save`, `index_add`, |
| `commit.h`         | Commit object interface              | Do not modify                                      |
| `commit.c`         | Commit creation and history          | Implement `commit_create`                          |
| `pes.c`            | CLI entry point and command dispatch | Do not modify                                      |
| `test_objects.c`   | Phase 1 test program                 | Do not modify                                      |
| `test_tree.c`      | Phase 2 test program                 | Do not modify                                      |
| `test_sequence.sh` | End-to-end integration test          | Do not modify                                      |
| `Makefile`         | Build system                         | Do not modify                                      |

---

‎commit.c‎
Original file line number	Diff line number	Diff line change
@@ -11,8 +11,8 @@
//
// Note: there is a blank line between the headers and the message.
//
// PROVIDED functions: commit_parse, commit_serialize, commit_walk
// TODO functions:     head_read, head_update, commit_create
// PROVIDED functions: commit_parse, commit_serialize, commit_walk, head_read, head_update
// TODO functions:     commit_create

#include "commit.h"
#include "index.h"
@@ -71,7 +71,7 @@ int commit_parse(const void *data, size_t len, Commit *commit_out) {
    return 0;
}

// Serialize a Commit struct to the text format described at the top of this file.
// Serialize a Commit struct to the text format.
// Caller must free(*data_out).
int commit_serialize(const Commit *commit, void **data_out, size_t *len_out) {
    char tree_hex[HASH_HEX_SIZE + 1];
@@ -101,7 +101,7 @@ int commit_serialize(const Commit *commit, void **data_out, size_t *len_out) {
    return 0;
}

// Walk commit history from HEAD to the root, calling `callback` per commit.
// Walk commit history from HEAD to the root.
int commit_walk(commit_walk_fn callback, void *ctx) {
    ObjectID id;
    if (head_read(&id) != 0) return -1;
@@ -125,67 +125,77 @@ int commit_walk(commit_walk_fn callback, void *ctx) {
    return 0;
}

// ─── TODO: Implement these ───────────────────────────────────────────────────
// Read the current HEAD commit hash.
//
// Steps:
//   1. Read the contents of .pes/HEAD
//   2. If it starts with "ref: " (symbolic reference):
//      a. Extract the ref path (e.g., "refs/heads/main")
//      b. Read .pes/<ref-path> to get the commit hash hex string
//      c. If that file doesn't exist, the branch has no commits → return -1
//   3. Otherwise, HEAD contains a raw commit hash (detached HEAD state)
//   4. Convert the 64-char hex string to an ObjectID using hex_to_hash()
//
// Returns 0 on success, -1 if no commits exist yet.
int head_read(ObjectID *id_out) {
    // TODO: Implement
    (void)id_out;
    return -1;
    FILE *f = fopen(HEAD_FILE, "r");
    if (!f) return -1;
    char line[512];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
    fclose(f);
    line[strcspn(line, "\r\n")] = '\0'; // strip newline
    char ref_path[512];
    if (strncmp(line, "ref: ", 5) == 0) {
        snprintf(ref_path, sizeof(ref_path), "%s/%s", PES_DIR, line + 5);
        f = fopen(ref_path, "r");
        if (!f) return -1; // Branch exists but has no commits yet
        if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
        fclose(f);
        line[strcspn(line, "\r\n")] = '\0';
    }
    return hex_to_hash(line, id_out);
}

// Update the current branch ref to point to a new commit.
//
// Steps:
//   1. Read .pes/HEAD to determine the current branch
//      (e.g., HEAD contains "ref: refs/heads/main" → update .pes/refs/heads/main)
//   2. Convert the new commit's ObjectID to hex
//   3. Write the hex hash to a temporary file (e.g., .pes/refs/heads/main.tmp)
//   4. fsync() the temp file
//   5. rename() the temp file to the ref file (atomic update)
//   6. fsync() the directory
//
// This is the "pointer swing" — the moment the commit becomes part of history.
//
// Returns 0 on success, -1 on error.
// Update the current branch ref to point to a new commit atomically.
int head_update(const ObjectID *new_commit) {
    // TODO: Implement
    (void)new_commit;
    return -1;
    FILE *f = fopen(HEAD_FILE, "r");
    if (!f) return -1;
    char line[512];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
    fclose(f);
    line[strcspn(line, "\r\n")] = '\0';
    char target_path[512];
    if (strncmp(line, "ref: ", 5) == 0) {
        snprintf(target_path, sizeof(target_path), "%s/%s", PES_DIR, line + 5);
    } else {
        snprintf(target_path, sizeof(target_path), "%s", HEAD_FILE); // Detached HEAD
    }
    char tmp_path[512];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", target_path);
    
    f = fopen(tmp_path, "w");
    if (!f) return -1;
    
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(new_commit, hex);
    fprintf(f, "%s\n", hex);
    
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    
    return rename(tmp_path, target_path);
}

// ─── TODO: Implement these ───────────────────────────────────────────────────
// Create a new commit from the current staging area.
//
// Steps:
//   1. Build a tree from the index using tree_from_index()
//      (this writes all tree objects and returns the root tree hash)
//   2. Read current HEAD to get the parent commit hash
//      (head_read returns -1 if this is the first commit — that's OK)
//   3. Fill in a Commit struct:
//      - tree = root tree hash from step 1
//      - parent = HEAD commit from step 2 (set has_parent accordingly)
//      - author = pes_author() (from pes.h)
//      - timestamp = current time (use time(NULL))
//      - message = the provided message string
//   4. Serialize the commit using commit_serialize()
//   5. Write to object store using object_write(OBJ_COMMIT, ...)
//   6. Update HEAD to point to the new commit using head_update()
//   7. Store the new commit's hash in *commit_id_out
// HINTS - Useful functions to call:
//   - tree_from_index   : writes the directory tree and gets the root hash
//   - head_read         : gets the parent commit hash (if any)
//   - pes_author        : retrieves the author name string (from pes.h)
//   - time(NULL)        : gets the current unix timestamp
//   - commit_serialize  : converts the filled Commit struct to a text buffer
//   - object_write      : saves the serialized text as OBJ_COMMIT
//   - head_update       : moves the branch pointer to your new commit
//
// Returns 0 on success, -1 on error.
int commit_create(const char *message, ObjectID *commit_id_out) {
    // TODO: Implement
    // TODO: Implement commit creation
    // (See Lab Appendix for logical steps)
    (void)message; (void)commit_id_out;
    return -1;
}
}
‎index.c‎
Original file line number	Diff line number	Diff line change
@@ -10,11 +10,10 @@
//
// This is intentionally a simple text format. No magic numbers, no
// binary parsing. The focus is on the staging area CONCEPT (tracking
// what will go into the next commit) and ATOMIC WRITES (temp+rename),
// not on binary format gymnastics.
// what will go into the next commit) and ATOMIC WRITES (temp+rename).
//
// PROVIDED functions: index_find, index_remove
// TODO functions:     index_load, index_save, index_add, index_status
// PROVIDED functions: index_find, index_remove, index_status
// TODO functions:     index_load, index_save, index_add

#include "index.h"
#include <stdio.h>
@@ -53,107 +52,124 @@ int index_remove(Index *index, const char *path) {
    return -1;
}

// Print the status of the working directory.
//
// Identifies files that are staged, unstaged (modified/deleted in working dir),
// and untracked (present in working dir but not in index).
// Returns 0.
int index_status(const Index *index) {
    printf("Staged changes:\n");
    int staged_count = 0;
    // Note: A true Git implementation deeply diffs against the HEAD tree here. 
    // For this lab, displaying indexed files represents the staging intent.
    for (int i = 0; i < index->count; i++) {
        printf("  staged:     %s\n", index->entries[i].path);
        staged_count++;
    }
    if (staged_count == 0) printf("  (nothing to show)\n");
    printf("\n");
    printf("Unstaged changes:\n");
    int unstaged_count = 0;
    for (int i = 0; i < index->count; i++) {
        struct stat st;
        if (stat(index->entries[i].path, &st) != 0) {
            printf("  deleted:    %s\n", index->entries[i].path);
            unstaged_count++;
        } else {
            // Fast diff: check metadata instead of re-hashing file content
            if (st.st_mtime != (time_t)index->entries[i].mtime_sec || st.st_size != (off_t)index->entries[i].size) {
                printf("  modified:   %s\n", index->entries[i].path);
                unstaged_count++;
            }
        }
    }
    if (unstaged_count == 0) printf("  (nothing to show)\n");
    printf("\n");
    printf("Untracked files:\n");
    int untracked_count = 0;
    DIR *dir = opendir(".");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            // Skip hidden directories, parent directories, and build artifacts
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            if (strcmp(ent->d_name, ".pes") == 0) continue;
            if (strcmp(ent->d_name, "pes") == 0) continue; // compiled executable
            if (strstr(ent->d_name, ".o") != NULL) continue; // object files
            // Check if file is tracked in the index
            int is_tracked = 0;
            for (int i = 0; i < index->count; i++) {
                if (strcmp(index->entries[i].path, ent->d_name) == 0) {
                    is_tracked = 1; 
                    break;
                }
            }
            
            if (!is_tracked) {
                struct stat st;
                stat(ent->d_name, &st);
                if (S_ISREG(st.st_mode)) { // Only list regular files for simplicity
                    printf("  untracked:  %s\n", ent->d_name);
                    untracked_count++;
                }
            }
        }
        closedir(dir);
    }
    if (untracked_count == 0) printf("  (nothing to show)\n");
    printf("\n");
    return 0;
}
// ─── TODO: Implement these ───────────────────────────────────────────────────

// Load the index from .pes/index.
//
// Steps:
//   1. If .pes/index does not exist, set index->count = 0 and return 0
//   2. Open the file for reading (fopen with "r")
//   3. For each line, parse the fields:
//      - Use fscanf or sscanf to read: mode, hex-hash, mtime, size, path
//      - Convert the 64-char hex hash to an ObjectID using hex_to_hash()
//   4. Populate index->entries and index->count
// HINTS - Useful functions:
//   - fopen (with "r"), fscanf, fclose : reading the text file line by line
//   - hex_to_hash                      : converting the parsed string to ObjectID
//
// Returns 0 on success, -1 on error.
int index_load(Index *index) {
    // TODO: Implement
    // TODO: Implement index loading
    // (See Lab Appendix for logical steps)
    (void)index;
    return -1;
}

// Save the index to .pes/index atomically.
//
// Steps:
//   1. Sort entries by path (use qsort with strcmp on the path field)
//   2. Open a temporary file for writing (.pes/index.tmp)
//   3. For each entry, write one line:
//      "<mode> <64-char-hex-hash> <mtime_sec> <size> <path>\n"
//      - Convert ObjectID to hex using hash_to_hex()
//   4. fflush() and fsync() the temp file to ensure data reaches disk
//   5. fclose() the temp file
//   6. rename(".pes/index.tmp", ".pes/index") — atomic replacement
//
// The rename() call is the key filesystem concept here: it is atomic
// on POSIX systems, meaning the index file is never in a half-written
// state even if the system crashes.
// HINTS - Useful functions and syscalls:
//   - qsort                            : sorting the entries array by path
//   - fopen (with "w"), fprintf        : writing to the temporary file
//   - hash_to_hex                      : converting ObjectID for text output
//   - fflush, fileno, fsync, fclose    : flushing userspace buffers and syncing to disk
//   - rename                           : atomically moving the temp file over the old index
//
// Returns 0 on success, -1 on error.
int index_save(const Index *index) {
    // TODO: Implement
    // TODO: Implement atomic index saving
    // (See Lab Appendix for logical steps)
    (void)index;
    return -1;
}

// Stage a file for the next commit.
//
// Steps:
//   1. Open and read the file at `path`
//   2. Write the file contents as a blob: object_write(OBJ_BLOB, ...)
//   3. Stat the file to get mode, mtime, and size
//      - Use stat() or lstat()
//      - mtime_sec = st.st_mtime
//      - size = st.st_size
//      - mode: use 0100755 if executable (st_mode & S_IXUSR), else 0100644
//   4. Search the index for an existing entry with this path (index_find)
//      - If found: update its hash, mode, mtime, and size
//      - If not found: append a new entry (check count < MAX_INDEX_ENTRIES)
//   5. Save the index to disk (index_save)
// HINTS - Useful functions and syscalls:
//   - fopen, fread, fclose             : reading the target file's contents
//   - object_write                     : saving the contents as OBJ_BLOB
//   - stat / lstat                     : getting file metadata (size, mtime, mode)
//   - index_find                       : checking if the file is already staged
//
// Returns 0 on success, -1 on error (file not found, etc.).
// Returns 0 on success, -1 on error.
int index_add(Index *index, const char *path) {
    // TODO: Implement
    // TODO: Implement file staging
    // (See Lab Appendix for logical steps)
    (void)index; (void)path;
    return -1;
}
// Print the status of the working directory.
//
// This involves THREE comparisons:
//
// 1. Index vs HEAD (staged changes):
//    - Load the HEAD commit's tree (if any commits exist)
//    - For each index entry, check if it exists in HEAD's tree with the same hash
//    - New in index but not in HEAD:       "new file:   <path>"
//    - In both but different hash:          "modified:   <path>"
//
// 2. Working directory vs index (unstaged changes):
//    - For each index entry, check the working directory file
//    - If file is missing:                  "deleted:    <path>"
//    - If file's mtime or size changed, recompute its hash:
//      - If hash differs from index:        "modified:   <path>"
//    - (If mtime+size unchanged, skip — assume file is unmodified)
//
// 3. Untracked files:
//    - Scan the working directory (skip .pes/)
//    - Any file not in the index:           "<path>"
//
// Expected output:
//   Staged changes:
//       new file:   hello.txt
//
//   Unstaged changes:
//       modified:   README.md
//
//   Untracked files:
//       notes.txt
//
// If a section has no entries, print the header followed by
//   (nothing to show)
//
// Returns 0.
int index_status(const Index *index) {
    // TODO: Implement
    (void)index;
    return -1;
}
}
‎object.c‎
Original file line number	Diff line number	Diff line change
@@ -77,6 +77,18 @@ int object_exists(const ObjectID *id) {
//   7. rename() the temp file to the final path (atomic on POSIX)
//   8. Open and fsync() the shard directory to persist the rename
//   9. Store the computed hash in *id_out
// HINTS - Useful syscalls and functions for this phase:
//   - sprintf / snprintf : formatting the header string
//   - compute_hash       : hashing the combined header + data
//   - object_exists      : checking for deduplication
//   - mkdir              : creating the shard directory (use mode 0755)
//   - open, write, close : creating and writing to the temp file
//                          (Use O_CREAT | O_WRONLY | O_TRUNC, mode 0644)
//   - fsync              : flushing the file descriptor to disk
//   - rename             : atomically moving the temp file to the final path
//
//
// Returns 0 on success, -1 on error.
int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out) {
@@ -96,6 +108,17 @@ int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out
//   5. Set *type_out to the parsed ObjectType
//   6. Allocate a buffer, copy the data portion (after the \0), set *data_out and *len_out
//
// HINTS - Useful syscalls and functions for this phase:
//   - object_path        : getting the target file path
//   - fopen, fread, fseek: reading the file into memory
//   - memchr             : safely finding the '\0' separating header and data
//   - strncmp            : parsing the type string ("blob", "tree", "commit")
//   - compute_hash       : re-hashing the read data for integrity verification
//   - memcmp             : comparing the computed hash against the requested hash
//   - malloc, memcpy     : allocating and returning the extracted data
//
// The caller is responsible for calling free(*data_out).
//
// The caller is responsible for calling free(*data_out).
// Returns 0 on success, -1 on error (file not found, corrupt, etc.).
int object_read(const ObjectID *id, ObjectType *type_out, void **data_out, size_t *len_out) {
‎pes.c‎
Original file line number	Diff line number	Diff line change
@@ -1,133 +1,61 @@
// pes.c — CLI entry point for PES-VCS
//
// This file dispatches user commands to the appropriate module functions.
//
// PROVIDED: cmd_init, cmd_add, cmd_status, cmd_log, main
// TODO:     cmd_commit (thin wrapper — the real logic is in commit.c)
#include "pes.h"
#include "tree.h"
#include "index.h"
#include "commit.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
// ─── PROVIDED ───────────────────────────────────────────────────────────────

// Helper: create a directory and all parent directories (like "mkdir -p").
static int mkdirs(const char *path, mode_t mode) {
    char tmp[512];
    char *p = NULL;
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, mode) != 0 && errno != EEXIST) return -1;
            *p = '/';
// ─── PROVIDED: Phase 5 Command Wrappers ─────────────────────────────────────
// Usage: 
//   pes branch          (lists branches)
//   pes branch <name>   (creates a branch)
//   pes branch -d <name>(deletes a branch)
void cmd_branch(int argc, char *argv[]) {
    if (argc == 2) {
        branch_list();
    } else if (argc == 3) {
        if (branch_create(argv[2]) == 0) {
            printf("Created branch '%s'\n", argv[2]);
        } else {
            fprintf(stderr, "error: failed to create branch '%s'\n", argv[2]);
        }
    }
    if (mkdir(tmp, mode) != 0 && errno != EEXIST) return -1;
    return 0;
}
void cmd_init(void) {
    if (mkdirs(PES_DIR, 0755) != 0)    { perror("mkdir .pes"); return; }
    if (mkdirs(OBJECTS_DIR, 0755) != 0) { perror("mkdir objects"); return; }
    if (mkdirs(REFS_DIR, 0755) != 0)    { perror("mkdir refs"); return; }
    FILE *f = fopen(HEAD_FILE, "w");
    if (!f) { perror("fopen HEAD"); return; }
    fprintf(f, "ref: refs/heads/main\n");
    fclose(f);
    printf("Initialized empty PES repository in .pes/\n");
}
void cmd_add(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: pes add <file>...\n");
        return;
    }
    Index index;
    if (index_load(&index) != 0) {
        fprintf(stderr, "error: failed to load index\n");
        return;
    }
    for (int i = 2; i < argc; i++) {
        if (index_add(&index, argv[i]) == 0) {
            printf("Added: %s\n", argv[i]);
    } else if (argc == 4 && strcmp(argv[2], "-d") == 0) {
        if (branch_delete(argv[3]) == 0) {
            printf("Deleted branch '%s'\n", argv[3]);
        } else {
            fprintf(stderr, "Failed to add: %s\n", argv[i]);
            fprintf(stderr, "error: failed to delete branch '%s'\n", argv[3]);
        }
    } else {
        fprintf(stderr, "Usage:\n  pes branch\n  pes branch <name>\n  pes branch -d <name>\n");
    }
}

void cmd_status(void) {
    Index index;
    if (index_load(&index) != 0) {
        fprintf(stderr, "error: failed to load index\n");
// Usage: pes checkout <branch_or_commit>
void cmd_checkout(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: pes checkout <branch_or_commit>\n");
        return;
    }
    index_status(&index);
}
static void log_callback(const ObjectID *id, const Commit *commit, void *ctx) {
    (void)ctx;
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id, hex);

    time_t t = (time_t)commit->timestamp;
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    printf("\033[33mcommit %s\033[0m\n", hex);
    printf("Author: %s\n", commit->author);
    printf("Date:   %s\n", timebuf);
    printf("\n    %s\n\n", commit->message);
}
void cmd_log(void) {
    if (commit_walk(log_callback, NULL) != 0) {
        fprintf(stderr, "error: no commits yet\n");
    const char *target = argv[2];
    if (checkout(target) == 0) {
        printf("Switched to '%s'\n", target);
    } else {
        fprintf(stderr, "error: checkout failed. Do you have uncommitted changes?\n");
    }
}

// ─── TODO: Implement this command wrapper ───────────────────────────────────
// Parse "-m <message>" from argv and call commit_create().
//
// Usage: pes commit -m "commit message"
//
// Steps:
//   1. Search argv for "-m". The next argument is the message string.
//      If "-m" is missing or has no argument after it, print:
//        "error: commit requires a message (-m \"message\")"
//      and return.
//   2. Call commit_create(message, &id)
//   3. On success, print: "Committed: <first-12-hex-chars>... <message>"
//   4. On failure, print: "error: commit failed"
void cmd_commit(int argc, char *argv[]) {
    // TODO: Implement
    (void)argc; (void)argv;
    fprintf(stderr, "error: commit not yet implemented\n");
}
// ─── PROVIDED: Command dispatch ─────────────────────────────────────────────

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: pes <command> [args]\n");
        fprintf(stderr, "\nCommands:\n");
        fprintf(stderr, "  init              Create a new PES repository\n");
        fprintf(stderr, "  add <file>...     Stage files for commit\n");
        fprintf(stderr, "  status            Show working directory status\n");
        fprintf(stderr, "  commit -m <msg>   Create a commit from staged files\n");
        fprintf(stderr, "  log               Show commit history\n");
        fprintf(stderr, "  init            Create a new PES repository\n");
        fprintf(stderr, "  add <file>...   Stage files for commit\n");
        fprintf(stderr, "  status          Show working directory status\n");
        fprintf(stderr, "  commit -m <msg> Create a commit from staged files\n");
        fprintf(stderr, "  log             Show commit history\n");
        fprintf(stderr, "  branch          List, create, or delete branches\n");
        fprintf(stderr, "  checkout <ref>  Switch branches or restore working tree\n");
        return 1;
    }

@@ -138,11 +66,13 @@ int main(int argc, char *argv[]) {
    else if (strcmp(cmd, "status") == 0)   cmd_status();
    else if (strcmp(cmd, "commit") == 0)   cmd_commit(argc, argv);
    else if (strcmp(cmd, "log") == 0)      cmd_log();
    else if (strcmp(cmd, "branch") == 0)   cmd_branch(argc, argv);
    else if (strcmp(cmd, "checkout") == 0) cmd_checkout(argc, argv);
    else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        fprintf(stderr, "Run 'pes' with no arguments for usage.\n");
        return 1;
    }

    return 0;
}
}
‎pes.h‎
Original file line number	Diff line number	Diff line change
@@ -6,6 +6,7 @@
#ifndef PES_H
#define PES_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
‎tree.c‎
Original file line number	Diff line number	Diff line change
@@ -1,7 +1,7 @@
// tree.c — Tree object serialization and construction
//
// PROVIDED functions: get_file_mode
// TODO functions:     tree_parse, tree_serialize, tree_from_index
// PROVIDED functions: get_file_mode, tree_parse, tree_serialize
// TODO functions:     tree_from_index
//
// Binary tree format (per entry, concatenated with no separators):
//   "<mode-as-ascii-octal> <name>\0<32-byte-binary-hash>"
@@ -34,77 +34,104 @@ uint32_t get_file_mode(const char *path) {
    return MODE_FILE;
}

// ─── TODO: Implement these ──────────────────────────────────────────────────
// Parse binary tree data into a Tree struct.
//
// The input `data` contains concatenated entries, each formatted as:
//   "<mode> <name>\0<32-byte-hash>"
// where <mode> is an ASCII octal string (e.g., "100644").
//
// Steps:
//   1. Start a pointer at the beginning of data
//   2. For each entry:
//      a. Read the mode string up to the space character
//      b. Read the name from after the space up to the null byte
//      c. Read the next 32 bytes as the raw binary hash
//      d. Store in tree_out->entries[tree_out->count++]
//   3. Stop when you've consumed all `len` bytes
//
// Parse binary tree data into a Tree struct safely.
// Returns 0 on success, -1 on parse error.
int tree_parse(const void *data, size_t len, Tree *tree_out) {
    // TODO: Implement
    (void)data; (void)len; (void)tree_out;
    return -1;
    tree_out->count = 0;
    const uint8_t *ptr = (const uint8_t *)data;
    const uint8_t *end = ptr + len;
    while (ptr < end && tree_out->count < MAX_TREE_ENTRIES) {
        TreeEntry *entry = &tree_out->entries[tree_out->count];
        // 1. Safely find the space character for the mode
        const uint8_t *space = memchr(ptr, ' ', end - ptr);
        if (!space) return -1; // Malformed data
        // Parse mode into an isolated buffer
        char mode_str[16] = {0};
        size_t mode_len = space - ptr;
        if (mode_len >= sizeof(mode_str)) return -1;
        memcpy(mode_str, ptr, mode_len);
        entry->mode = strtol(mode_str, NULL, 8);
        ptr = space + 1; // Skip space
        // 2. Safely find the null terminator for the name
        const uint8_t *null_byte = memchr(ptr, '\0', end - ptr);
        if (!null_byte) return -1; // Malformed data
        size_t name_len = null_byte - ptr;
        if (name_len >= sizeof(entry->name)) return -1;
        memcpy(entry->name, ptr, name_len);
        entry->name[name_len] = '\0'; // Ensure null-terminated
        ptr = null_byte + 1; // Skip null byte
        // 3. Read the 32-byte binary hash
        if (ptr + HASH_SIZE > end) return -1; 
        memcpy(entry->hash.hash, ptr, HASH_SIZE);
        ptr += HASH_SIZE;
        tree_out->count++;
    }
    return 0;
}
// Helper for qsort to ensure consistent tree hashing
static int compare_tree_entries(const void *a, const void *b) {
    return strcmp(((const TreeEntry *)a)->name, ((const TreeEntry *)b)->name);
}

// Serialize a Tree struct into binary format for storage.
//
// Steps:
//   1. Sort entries by name (required for deterministic hashing —
//      same directory contents must always produce the same tree hash)
//   2. Calculate total output size
//   3. Allocate output buffer
//   4. For each entry, write: "<mode> <name>\0<32-byte-hash>"
//   5. Set *data_out and *len_out
//
// Caller must free(*data_out).
// Returns 0 on success, -1 on error.
int tree_serialize(const Tree *tree, void **data_out, size_t *len_out) {
    // TODO: Implement
    (void)tree; (void)data_out; (void)len_out;
    return -1;
    // Estimate max size: (6 bytes mode + 1 byte space + 256 bytes name + 1 byte null + 32 bytes hash) per entry
    size_t max_size = tree->count * 296; 
    uint8_t *buffer = malloc(max_size);
    if (!buffer) return -1;
    // Create a mutable copy to sort entries (Git requirement)
    Tree sorted_tree = *tree;
    qsort(sorted_tree.entries, sorted_tree.count, sizeof(TreeEntry), compare_tree_entries);
    size_t offset = 0;
    for (int i = 0; i < sorted_tree.count; i++) {
        const TreeEntry *entry = &sorted_tree.entries[i];
        
        // Write mode and name (%o writes octal correctly for Git standards)
        int written = sprintf((char *)buffer + offset, "%o %s", entry->mode, entry->name);
        offset += written + 1; // +1 to step over the null terminator written by sprintf
        
        // Write binary hash
        memcpy(buffer + offset, entry->hash.hash, HASH_SIZE);
        offset += HASH_SIZE;
    }
    *data_out = buffer;
    *len_out = offset;
    return 0;
}

// ─── TODO: Implement these ──────────────────────────────────────────────────
// Build a tree hierarchy from the current index and write all tree
// objects to the object store.
//
// The index contains flat paths like:
//   "README.md"
//   "src/main.c"
//   "src/util/helper.c"
//
// You must construct a tree hierarchy:
//   root tree:
//     100644 blob <hash> README.md
//     040000 tree <hash> src
//   src tree:
//     100644 blob <hash> main.c
//     040000 tree <hash> util
//   util tree:
//     100644 blob <hash> helper.c
//
// Steps:
//   1. Load the index (use index_load from index.h)
//   2. Group entries by their top-level directory component
//   3. For files at the current level, add blob entries to the tree
//   4. For files in subdirectories, recursively build subtrees
//   5. Serialize and write each tree object using object_write(OBJ_TREE, ...)
//   6. Return the root tree's hash in *id_out
// HINTS - Useful functions and concepts for this phase:
//   - index_load      : load the staged files into memory
//   - strchr          : find the first '/' in a path to separate directories from files
//   - strncmp         : compare prefixes to group files belonging to the same subdirectory
//   - Recursion       : you will likely want to create a recursive helper function 
//                       (e.g., `write_tree_level(entries, count, depth)`) to handle nested dirs.
//   - tree_serialize  : convert your populated Tree struct into a binary buffer
//   - object_write    : save that binary buffer to the store as OBJ_TREE
//
// Returns 0 on success, -1 on error.
int tree_from_index(ObjectID *id_out) {
    // TODO: Implement
    // TODO: Implement recursive tree building
    // (See Lab Appendix for logical steps)
    (void)id_out;
    return -1;
}
}
