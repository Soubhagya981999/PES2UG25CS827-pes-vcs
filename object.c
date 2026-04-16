Customizable line height
The default line height has been increased for improved accessibility. You can choose to enable a more compact line height from the view settings menu.

‎README.md‎


Original file line number	Diff line number	Diff line change
@@ -53,7 +53,7 @@ If unset, it defaults to `"PES User <pes@localhost>"`.
| `tree.h`           | Tree object interface                | Do not modify                                      |
| `tree.c`           | Tree serialization and construction  | Implement `tree_from_index`                        |
| `index.h`          | Staging area interface               | Do not modify                                      |
| `index.c`          | Staging area (text-based index file) | Implement `index_load`, `index_save`, `index_add`, |
| `index.c`          | Staging area (text-based index file) | Implement `index_load`, `index_save`, `index_add`  |
| `commit.h`         | Commit object interface              | Do not modify                                      |
| `commit.c`         | Commit creation and history          | Implement `commit_create`                          |
| `pes.c`            | CLI entry point and command dispatch | Do not modify                                      |
@@ -420,7 +420,7 @@ The test program verifies:

### What to Implement

Open `index.c`. Four functions are marked `// TODO`:
Open `index.c`. Three functions are marked `// TODO`:

1. **`index_load`** — Reads the text-based `.pes/index` file into an `Index` struct.
   - If the file doesn't exist, initializes an empty index (this is not an error)
@@ -439,15 +439,15 @@ Open `index.c`. Four functions are marked `// TODO`:

```
Staged changes:
    new file:   hello.txt
    modified:   src/main.c
  staged:     hello.txt
  staged:     src/main.c
Unstaged changes:
    modified:   README.md
    deleted:    old_file.txt
  modified:   README.md
  deleted:    old_file.txt
Untracked files:
    notes.txt
  untracked:  notes.txt
```

If a section has no entries, print the header followed by `(nothing to show)`.
@@ -474,24 +474,19 @@ cat .pes/index # Human-readable text format

**Filesystem Concepts:** Linked structures on disk, reference files, atomic pointer updates

**Files:** `commit.h` (read), `commit.c` (implement all TODO functions), `pes.c` (implement `cmd_commit`)
**Files:** `commit.h` (read), `commit.c` (implement all TODO functions)

### What to Implement

Open `commit.c`. Three functions are marked `// TODO`:
Open `commit.c`. One function is marked `// TODO`:

1. **`commit_create`** — The main commit function:
   - Builds a tree from the index using `tree_from_index()` (**not** from the working directory — commits snapshot the staged state)
   - Reads current HEAD as the parent (may not exist for first commit)
   - Gets the author string from `pes_author()` (defined in `pes.h`)
   - Writes the commit object, then updates HEAD

`commit_parse`, `commit_serialize`, and `commit_walk` are already implemented — read them to understand the commit format before writing `commit_create`.
Also implement **`cmd_commit`** in `pes.c`:
- Parse `-m <message>` from command-line arguments
- If `-m` is missing, print: `error: commit requires a message (-m "message")`
- On success, print: `Committed: <first-12-hex-chars>... <message>`
`commit_parse`, `commit_serialize`, `commit_walk`, `head_read`, and `head_update` are already implemented — read them to understand the commit format before writing `commit_create`.

The commit text format is specified in the comment at the top of `commit.c`.

@@ -573,7 +568,6 @@ The following questions cover filesystem concepts beyond the implementation scop
| `tree.c`       | Tree serialization and construction      |
| `index.c`      | Staging area implementation              |
| `commit.c`     | Commit creation and history walking      |
| `pes.c`        | CLI entry point with `cmd_commit`        |

### Analysis Questions (written answers)

‎commit.c‎
Original file line number	Diff line number	Diff line change
@@ -61,13 +61,13 @@ int commit_parse(const void *data, size_t len, Commit *commit_out) {
    if (!last_space) return -1;
    ts = (uint64_t)strtoull(last_space + 1, NULL, 10);
    *last_space = '\0';
    strncpy(commit_out->author, author_buf, sizeof(commit_out->author) - 1);
    snprintf(commit_out->author, sizeof(commit_out->author), "%s", author_buf);
    commit_out->timestamp = ts;
    p = strchr(p, '\n') + 1;  // skip author line
    p = strchr(p, '\n') + 1;  // skip committer line
    p = strchr(p, '\n') + 1;  // skip blank line

    strncpy(commit_out->message, p, sizeof(commit_out->message) - 1);
    snprintf(commit_out->message, sizeof(commit_out->message), "%s", p);
    return 0;
}

@@ -155,14 +155,14 @@ int head_update(const ObjectID *new_commit) {
    fclose(f);
    line[strcspn(line, "\r\n")] = '\0';

    char target_path[512];
    char target_path[520];
    if (strncmp(line, "ref: ", 5) == 0) {
        snprintf(target_path, sizeof(target_path), "%s/%s", PES_DIR, line + 5);
    } else {
        snprintf(target_path, sizeof(target_path), "%s", HEAD_FILE); // Detached HEAD
    }

    char tmp_path[512];
    char tmp_path[528];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", target_path);

    f = fopen(tmp_path, "w");
‎index.h‎
Original file line number	Diff line number	Diff line change
@@ -42,16 +42,15 @@ IndexEntry* index_find(Index *index, const char *path);

// Print the status of the working directory compared to the index and HEAD.
// Output format:
//   Staged changes (index vs HEAD):
//     new file:   <path>
//     modified:   <path>
//   Staged changes:
//     staged:     <path>
//
//   Unstaged changes (working dir vs index):
//   Unstaged changes:
//     modified:   <path>
//     deleted:    <path>
//
//   Untracked files:
//     <path>
//     untracked:  <path>
int index_status(const Index *index);

#endif // INDEX_H
‎object.c‎
Original file line number	Diff line number	Diff line change
@@ -15,7 +15,7 @@
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

// ─── PROVIDED ────────────────────────────────────────────────────────────────

@@ -37,10 +37,12 @@ int hex_to_hash(const char *hex, ObjectID *id_out) {
}

void compute_hash(const void *data, size_t len, ObjectID *id_out) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, len);
    SHA256_Final(id_out->hash, &ctx);
    unsigned int hash_len;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, id_out->hash, &hash_len);
    EVP_MD_CTX_free(ctx);
}

// Get the filesystem path where an object should be stored.
@@ -118,8 +120,6 @@ int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out
//   - malloc, memcpy     : allocating and returning the extracted data
//
// The caller is responsible for calling free(*data_out).
//
// The caller is responsible for calling free(*data_out).
// Returns 0 on success, -1 on error (file not found, corrupt, etc.).
int object_read(const ObjectID *id, ObjectType *type_out, void **data_out, size_t *len_out) {
    // TODO: Implement
‎pes.c‎
Original file line number	Diff line number	Diff line change
@@ -1,45 +1,103 @@
// pes.c — CLI entry point and command dispatch
//
// This file is PROVIDED. Do not modify.
#include "pes.h"
#include "index.h"
#include "commit.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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
    } else if (argc == 4 && strcmp(argv[2], "-d") == 0) {
        if (branch_delete(argv[3]) == 0) {
            printf("Deleted branch '%s'\n", argv[3]);
        } else {
            fprintf(stderr, "error: failed to delete branch '%s'\n", argv[3]);
// ─── PROVIDED: Command Implementations ──────────────────────────────────────
// Usage: pes init
void cmd_init(void) {
    if (mkdir(PES_DIR, 0755) != 0 && access(PES_DIR, F_OK) != 0) {
        fprintf(stderr, "error: failed to create %s\n", PES_DIR);
        return;
    }
    mkdir(OBJECTS_DIR, 0755);
    mkdir(".pes/refs", 0755);
    mkdir(REFS_DIR, 0755);
    if (access(HEAD_FILE, F_OK) != 0) {
        FILE *f = fopen(HEAD_FILE, "w");
        if (f) {
            fprintf(f, "ref: refs/heads/main\n");
            fclose(f);
        }
    } else {
        fprintf(stderr, "Usage:\n  pes branch\n  pes branch <name>\n  pes branch -d <name>\n");
    }
    printf("Initialized empty PES repository in %s/\n", PES_DIR);
}

// Usage: pes checkout <branch_or_commit>
void cmd_checkout(int argc, char *argv[]) {
// Usage: pes add <file>...
void cmd_add(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: pes checkout <branch_or_commit>\n");
        fprintf(stderr, "Usage: pes add <file>...\n");
        return;
    }

    const char *target = argv[2];
    if (checkout(target) == 0) {
        printf("Switched to '%s'\n", target);
    } else {
        fprintf(stderr, "error: checkout failed. Do you have uncommitted changes?\n");
    Index index;
    if (index_load(&index) != 0) {
        fprintf(stderr, "error: failed to load index\n");
        return;
    }
    for (int i = 2; i < argc; i++) {
        if (index_add(&index, argv[i]) != 0) {
            fprintf(stderr, "error: failed to add '%s'\n", argv[i]);
        }
    }
}
// Usage: pes status
void cmd_status(void) {
    Index index;
    if (index_load(&index) != 0) {
        fprintf(stderr, "error: failed to load index\n");
        return;
    }
    index_status(&index);
}
// Usage: pes commit -m <message>
void cmd_commit(int argc, char *argv[]) {
    if (argc < 4 || strcmp(argv[2], "-m") != 0) {
        fprintf(stderr, "error: commit requires a message (-m \"message\")\n");
        return;
    }
    const char *message = argv[3];
    ObjectID commit_id;
    if (commit_create(message, &commit_id) != 0) {
        fprintf(stderr, "error: commit failed\n");
        return;
    }
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(&commit_id, hex);
    printf("Committed: %.12s... %s\n", hex, message);
}
// Callback for commit_walk used by cmd_log.
static void print_commit(const ObjectID *id, const Commit *commit, void *ctx) {
    (void)ctx;
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id, hex);
    printf("commit %s\n", hex);
    printf("Author: %s\n", commit->author);
    printf("Date:   %llu\n", (unsigned long long)commit->timestamp);
    printf("\n    %s\n\n", commit->message);
}
// Usage: pes log
void cmd_log(void) {
    if (commit_walk(print_commit, NULL) != 0) {
        fprintf(stderr, "No commits yet.\n");
    }
}

@@ -54,8 +112,6 @@ int main(int argc, char *argv[]) {
        fprintf(stderr, "  status          Show working directory status\n");
        fprintf(stderr, "  commit -m <msg> Create a commit from staged files\n");
        fprintf(stderr, "  log             Show commit history\n");
        fprintf(stderr, "  branch          List, create, or delete branches\n");
        fprintf(stderr, "  checkout <ref>  Switch branches or restore working tree\n");
        return 1;
    }

@@ -66,13 +122,11 @@ int main(int argc, char *argv[]) {
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
@@ -1,7 +1,6 @@
// pes.h — Core data structures and constants for PES-VCS
//
// This file is PROVIDED. Do not modify unless adding helper declarations
// for your own utility functions.
// This file is PROVIDED. Do not modify.

#ifndef PES_H
#define PES_H
‎test_objects.c‎
Original file line number	Diff line number	Diff line change
@@ -84,8 +84,9 @@ void test_integrity(void) {

int main(void) {
    // Clean slate
    system("rm -rf .pes");
    system("mkdir -p .pes/objects .pes/refs/heads");
    int rc __attribute__((unused));
    rc = system("rm -rf .pes");
    rc = system("mkdir -p .pes/objects .pes/refs/heads");

    test_blob_storage();
    test_deduplication();
‎test_sequence.sh‎
Original file line number	Diff line number	Diff line change
@@ -8,7 +8,7 @@

set -euo pipefail

PES="./pes"
PES="$(cd "$(dirname "$0")" && pwd)/pes"
TEST_DIR="$(mktemp -d)"

cleanup() {
‎test_tree.c‎
Original file line number	Diff line number	Diff line change
@@ -98,8 +98,9 @@ void test_tree_determinism(void) {
}

int main(void) {
    system("rm -rf .pes");
    system("mkdir -p .pes/objects .pes/refs/heads");
    int rc __attribute__((unused));
    rc = system("rm -rf .pes");
    rc = system("mkdir -p .pes/objects .pes/refs/heads");

    test_tree_roundtrip();
    test_tree_determinism();
