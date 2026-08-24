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
  { chapter: '8 · Heap', items: [
    { id: 'heap_heapify',   name: 'Heapify',        view: 'tree', time: 'O(log n)', space: 'O(log n)' },
    { id: 'heap_insert',    name: 'Insert',         view: 'tree', time: 'O(log n)', space: 'O(1)' },
    { id: 'heap_delete',    name: 'Delete',         view: 'tree', time: 'O(log n)', space: 'O(log n)' },
  ]},
  { chapter: '9 · Grafi', items: [
    { id: 'graph_repr',     name: 'Matrice e lista', view: 'graph', time: 'O(1) / O(grado)', space: 'O(V²) / O(V+E)' },
    { id: 'graph_bfs',      name: 'Breadth-First Search', view: 'graph', time: 'O(V + E)', space: 'O(V)' },
    { id: 'graph_dfs',      name: 'Depth-First Search',   view: 'graph', time: 'O(V + E)', space: 'O(V)' },
  ]},
  { chapter: '10 · Cammini minimi', items: [
    { id: 'dijkstra',       name: 'Dijkstra',       view: 'graph',  time: 'O(V²)',  space: 'O(V)' },
    { id: 'floyd_warshall', name: 'Floyd-Warshall', view: 'matrix', time: 'O(V³)',  space: 'O(V²)' },
    { id: 'bellman_ford',   name: 'Bellman-Ford',   view: 'graph',  time: 'O(V·E)', space: 'O(V)' },
    { id: 'a_star',         name: 'A*',             view: 'graph',  time: 'O(E)',   space: 'O(V)' },
  ]},
  { chapter: '11 · Flusso e MST', items: [
    { id: 'ford_fulkerson', name: 'Ford-Fulkerson', view: 'graph', time: 'O(E · f)',    space: 'O(V²)' },
    { id: 'kruskal',        name: 'Kruskal',        view: 'graph', time: 'O(E log E)',  space: 'O(V)' },
  ]},
  { chapter: '12 · Scheduling', items: [
    { id: 'fcfs',                name: 'First Come First Served', view: 'gantt', time: 'O(n log n)', space: 'O(n)' },
    { id: 'sjf',                 name: 'Shortest Job First',      view: 'gantt', time: 'O(n²)',     space: 'O(n)' },
    { id: 'priority_scheduling', name: 'Priority Scheduling',     view: 'gantt', time: 'O(n²)',     space: 'O(n)' },
    { id: 'round_robin',         name: 'Round Robin',             view: 'gantt', time: 'O(n)',      space: 'O(n)' },
    { id: 'scheduling_compare',  name: 'Confronto fra i quattro', view: 'table', time: '—',         space: '—' },
  ]},
  { chapter: '13 · Sincronizzazione', items: [
    { id: 'lock_variable',       name: 'Lock variable',       view: 'threads', time: '—', space: 'O(1)' },
    { id: 'petersons_solution',  name: "Peterson's Solution", view: 'threads', time: '—', space: 'O(1)' },
    { id: 'semaphores',          name: 'Semafori',            view: 'threads', time: 'O(1)', space: 'O(n)' },
  ]},
];
