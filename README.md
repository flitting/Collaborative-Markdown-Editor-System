# 1. Overview

This project implements a collaborative Markdown-based document editing system in C using a client-server architecture.

The system consists of:

* `server.c` — Authoritative document host
* `client.c` — Console-based client editor

The server maintains the true document state, processes all edit commands atomically, and broadcasts updates at fixed time intervals. Clients maintain synchronized local copies.

This project is based on an assignment during my study.



---

# 2. Design Decisions

## 2.1 Document Data Structure

To avoid costly memory shifting during insertions and deletions, the document is implemented using:

> **Doubly linked list of character nodes**

### Justification

* Insertions and deletions are O(1) once the cursor position is located.
* No large `memmove()` operations required.
* Efficient for frequent mid-document edits.
* Dynamic memory allows arbitrary document size.
* Fully freed on teardown.

This directly answers Short Answer Q1:

> We avoid copying and shifting large memory chunks by using a linked structure instead of a contiguous array.

---

## 2.2 Versioning Model

* Version starts at 0.
* Version increments only if at least one valid edit occurred.
* Broadcast occurs every TIME_INTERVAL milliseconds.
* Each client command references a specific version.

### Command Handling

Each command is stored in a per-client queue with:

* Timestamp
* Username
* Version number
* Command string

At broadcast time:

1. Server collects all queued commands.
2. Sorts by timestamp.
3. Executes sequentially.
4. Sends:

```
VERSION <n>
EDIT <user> <command> SUCCESS/Reject <reason>
...
END
```

This ensures deterministic ordering.

---

## 2.3 Handling Cursor Adjustments (Overlapping Edits)

If earlier deletions affect later commands:

### Single position commands:

* Cursor inside deleted region → move to deletion start.

### Range commands:

* Both inside deleted region → `DELETED_POSITION`
* One inside → clamp to nearest valid edge.

### Precedence:

* `INVALID_POSITION` takes precedence over `DELETED_POSITION`.

This ensures the edit reflects the user’s original intention (Short Answer Q2).

---

# 3. Multi-Client Concurrency

## Thread Model

* One POSIX thread per client.
* Each thread:

  * Manages FIFO communication
  * Queues commands
* Main thread:

  * Performs periodic version commit
  * Applies queued commands in timestamp order
  * Broadcasts result

### Why this is safe:

* No two edits occur at the exact same timestamp (guaranteed).
* Shared document protected using mutex.
* Command queues protected via per-thread synchronization.

---

# 4. Testing Strategy

## 4.1 Unit Tests Performed

### Basic Insert

* Insert at start
* Insert at end
* Insert in middle

### Delete

* Delete within bounds
* Delete beyond document end
* Delete entire document

### Formatting

* Heading levels 1–3
* Bold & Italic overlapping
* Inline code
* Blockquote
* Ordered list renumbering
* Horizontal rule newline enforcement

### Version Tests

* Edit correct version → SUCCESS
* Edit outdated version → OUTDATED_VERSION
* Accept N and N-1 versions

---

## 4.2 Concurrency Tests (Multiple Clients)

Test Case 1 – Simultaneous Inserts

Client A inserts at position 0
Client B inserts at position 0

Expected:

* Earlier timestamp appears first.
* Later insert shifts correctly.

---

Test Case 2 – Delete + Insert Overlap

Client A deletes range 3–9
Client B inserts at position 5

Expected:

* Insert adjusted to deletion start.

---

Test Case 3 – Range Inside Deleted Area

Delete 3–9
Then BOLD 4–6

Expected:

* DELETED_POSITION

---

Test Case 4 – Ordered List Renumbering

Insert ordered list in middle of list.
Verify renumbering remains consistent.

---

These tests specifically target:

* Race conditions
* Timestamp ordering
* Cursor adjustment logic
* Ordered list repair logic

(Short Answer Q3)

---

# 5. Ordered List Handling

Ordered list renumbering is performed by:

1. Detect contiguous ordered list block.
2. Recalculate numbering from 1 upward.
3. Stop at first non-ordered item or newline break.

Deletion triggers renumber repair during commit phase.

Nested lists are not supported (as per safe assumptions).

(Short Answer Q4)

---

# 6.  Extension


Server accepts edits targeting:

* Current version N
* Previous version N-1

Implementation:

* If command version == N → process normally.
* If command version == N-1:

  * Apply cursor transformation to align with current document.
  * Then process.

This behaviour is well-defined because:

* All version deltas are tracked.
* Cursor adjustments are deterministic.
* Conflicts resolved using timestamp ordering.

Edge cases tested:

* Multiple overlapping deletes across N-1 edits.
* Insert on region modified in N.

---

# 7. Memory Management

* All nodes dynamically allocated.
* Freed on:

  * Client disconnect
  * Server shutdown
  * Document teardown
* No memory leaks (validated via valgrind).
* No VLA usage.
* No shared memory.
* No regex.

---

# 8. Performance Considerations

Compared to a single-process single-thread model:

* Multi-threading allows concurrent client communication.
* Document commit still serialized → ensures consistency.
* Non-blocking behaviour prevents client starvation.
* Realtime signals avoid signal loss.

(Short Answer Q5 & Q6)

Using `select()`/`epoll()`:

* Could replace per-client threads.
* Would reduce thread overhead.
* But assignment explicitly requires one POSIX thread per client.

---

# 9. Compliance With Restrictions

✔ Written entirely in C
✔ Uses POSIX threads
✔ Uses dynamic memory
✔ No external libraries beyond libc
✔ No VLAs
✔ No shared memory
✔ No regex
✔ Clean git history
✔ No return code 42 used
✔ English comments only

---

# 10. How to Compile

```
gcc -Wall -Wextra -pthread server.c -o server
gcc -Wall -Wextra -pthread client.c -o client
```

---

# 11. How to Run

Start server:

```
./server <TIME_INTERVAL>
```

Start client:

```
./client <server_pid> <username>
```

---

# 12. Teardown Behaviour

* If no clients connected:

  * `QUIT` shuts down server.
  * Saves document as `doc.md`.
* If clients still connected:

  * QUIT rejected with count.

---

