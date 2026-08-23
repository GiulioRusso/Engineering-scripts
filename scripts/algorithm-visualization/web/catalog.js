// catalog.js - metadata only, one entry per algorithm.
//
// With dozens of algorithms the traces are far too big to load all at once, so
// this file holds just what the sidebar needs; the selected algorithm's trace
// is injected as a <script> on demand.
window.CATALOG = [
  { chapter: '1 · Array', items: [
    { id: 'array_access',   name: 'Accessing',      view: 'array', time: 'O(1)',     space: 'O(1)' },
    { id: 'array_insert',   name: 'Insert',         view: 'array', time: 'O(n)',     space: 'O(1)' },
    { id: 'array_delete',   name: 'Delete',         view: 'array', time: 'O(n)',     space: 'O(1)' },
    { id: 'linear_search',  name: 'Linear Search',  view: 'array', time: 'O(n)',     space: 'O(1)' },
    { id: 'binary_search',  name: 'Binary Search',  view: 'array', time: 'O(log n)', space: 'O(1)' },
  ]},
  { chapter: '2 · Ordinamento', items: [
    { id: 'insertion_sort', name: 'Insertion Sort', view: 'array', time: 'O(n²)',    space: 'O(1)' },
    { id: 'selection_sort', name: 'Selection Sort', view: 'array', time: 'O(n²)',    space: 'O(1)' },
    { id: 'bubble_sort',    name: 'Bubble Sort',    view: 'array', time: 'O(n²)',    space: 'O(1)' },
    { id: 'merge_sort',     name: 'Merge Sort',     view: 'array', time: 'O(n log n)', space: 'O(n)' },
    { id: 'quick_sort',     name: 'Quick Sort',     view: 'array', time: 'O(n log n)', space: 'O(log n)' },
  ]},
  { chapter: '3 · Altro', items: [
    { id: 'kadane',         name: 'Kadane',         view: 'array', time: 'O(n)',     space: 'O(1)' },
  ]},
  { chapter: '4 · Stack e Queue', items: [
    { id: 'stack',          name: 'Stack',          view: 'stack', time: 'O(1)',     space: 'O(n)' },
    { id: 'queue',          name: 'Queue circolare', view: 'queue', time: 'O(1)',    space: 'O(n)' },
  ]},
  { chapter: '5 · Linked List', items: [
    { id: 'list_access',    name: 'Accessing',      view: 'linkedlist', time: 'O(n)', space: 'O(1)' },
    { id: 'list_insert',    name: 'Insert',         view: 'linkedlist', time: 'O(1)–O(n)', space: 'O(1)' },
    { id: 'list_delete',    name: 'Delete',         view: 'linkedlist', time: 'O(1)–O(n)', space: 'O(1)' },
    { id: 'list_search',    name: 'Search',         view: 'linkedlist', time: 'O(n)', space: 'O(1)' },
    { id: 'list_types',     name: 'Singly / Doubly / Circular', view: 'linkedlist', time: '—', space: 'O(n)' },
  ]},
  { chapter: '6 · Tree', items: [
    { id: 'tree_basics',    name: 'Tree n-ario',    view: 'tree', time: 'O(n)', space: 'O(h)' },
    { id: 'tree_bfs',       name: 'Breadth-First',  view: 'tree', time: 'O(n)', space: 'O(w)' },
    { id: 'tree_preorder',  name: 'Pre-order',      view: 'tree', time: 'O(n)', space: 'O(h)' },
    { id: 'tree_inorder',   name: 'In-order',       view: 'tree', time: 'O(n)', space: 'O(h)' },
    { id: 'tree_postorder', name: 'Post-order',     view: 'tree', time: 'O(n)', space: 'O(h)' },
  ]},
  { chapter: '7 · Binary Search Tree', items: [
    { id: 'bst_search',     name: 'Search',         view: 'tree', time: 'O(h)', space: 'O(1)' },
    { id: 'bst_insert',     name: 'Insertion',      view: 'tree', time: 'O(h)', space: 'O(h)' },
    { id: 'bst_delete',     name: 'Delete (3 casi)', view: 'tree', time: 'O(h)', space: 'O(h)' },
  ]},
];
