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
];
