# AVL Tree with Lazy Range Updates

A C++20 implementation of an AVL tree (self-balancing BST) that supports efficient range updates via lazy propagation. Themed around managing an army of hobbits with stats.

## Data Structure

Each node stores a `Hobbit` with a name (used as a key, ordered A-Z), and three stats: HP, offense, and defense. The tree maintains AVL balance invariants through single and double rotations on insert and delete.

### Supported Operations

| Operation | Time Complexity | Description |
|-----------|----------------|-------------|
| `add` | O(log n) | Insert a hobbit (rejects duplicates and HP <= 0) |
| `erase` | O(log n) | Remove a hobbit by name, returns its stats |
| `stats` | O(log n) | Look up a hobbit's current stats by name |
| `enchant` | O(log n + k) | Apply stat deltas to all hobbits in a lexicographic name range, with atomicity -- the operation is rejected if any hobbit's HP would drop to zero or below |
| `for_each` | O(n) | In-order traversal over all hobbits |

### Lazy Propagation

Range enchantments are applied lazily. When an `enchant` covers an entire subtree, a pending delta is stored on the subtree root instead of visiting every node. Pending deltas are pushed down to children on demand (during lookups, insertions, deletions, and rotations), keeping range updates efficient.
