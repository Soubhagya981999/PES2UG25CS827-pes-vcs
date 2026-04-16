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
