// catalog.js - metadata only, one entry per circuit.
//
// The selected circuit's trace is injected as a <script> on demand; this file
// holds just what the sidebar needs. Badges repurpose the algorithm
// visualizer's time/space slots: `time` carries propagation delay, `space`
// carries gate count.
window.CATALOG = [
  { chapter: '1 · Logic Gates', items: [
    { id: 'gates_truth',     name: 'The Seven Gates',     view: 'schematic', time: '1 t_p', space: '1 gate' },
    { id: 'gates_timing',    name: 'Gates Over Time',     view: 'waveform',  time: '1 t_p', space: '1 gate' },
    { id: 'nand_universal',  name: 'NAND Is Universal',   view: 'schematic', time: '2 t_p', space: '3 NAND' },
    { id: 'tri_state',       name: 'Tri-State Buffer',    view: 'schematic', time: '1 t_p', space: '1 buffer' },
  ]},
  { chapter: '2 · Adders', items: [
    { id: 'half_adder',          name: 'Half Adder',                    view: 'schematic', time: '2 t_p',      space: '2 gate' },
    { id: 'full_adder',          name: 'Full Adder',                    view: 'schematic', time: '3 t_p',      space: '5 gate' },
    { id: 'ripple_carry',        name: 'Ripple-Carry, 4 bit',           view: 'schematic', time: '4 t_p',      space: '4 FA' },
    { id: 'adder_delay_compare', name: 'Ripple vs Carry-Look-Ahead',    view: 'table',      time: 'O(n)',       space: 'O(1)' },
    { id: 'comparator_1bit',     name: '1-Bit Comparator',              view: 'schematic', time: '2 t_p',      space: '5 gate' },
    { id: 'parity_tree',         name: 'Parity Generator',              view: 'schematic', time: '2 t_p',      space: '3 XOR' },
  ]},
  { chapter: '3 · Latches', items: [
    { id: 'sr_latch_nor',          name: 'SR Latch (NOR)',         view: 'schematic', time: 'level-sensitive', space: '2 NOR' },
    { id: 'sr_latch_timing',       name: 'SR Over Time',           view: 'waveform',  time: 'level-sensitive', space: '2 NOR' },
    { id: 'd_latch',                name: 'D Latch with Enable',   view: 'schematic', time: 'level-sensitive', space: '4 NAND' },
    { id: 'd_latch_transparency',   name: 'Transparency',          view: 'waveform',  time: 'level-sensitive', space: '4 NAND' },
    { id: 'sr_nor_vs_nand',         name: 'NOR vs NAND: Duality',  view: 'schematic', time: 'level-sensitive', space: '2+2 gates' },
    { id: 'latch_glitch',           name: 'The Glitch That Survives', view: 'waveform', time: 'level-sensitive', space: '4 NAND' },
  ]},
  { chapter: '4 · Flip-Flops', items: [
    { id: 'sr_flipflop',             name: 'SR Flip-Flop',              view: 'schematic', time: 'edge-triggered', space: '2 latches' },
    { id: 'd_flipflop_master_slave', name: 'D Master-Slave',            view: 'waveform',  time: 'edge-triggered', space: '2 D-latches' },
    { id: 'latch_vs_flipflop',       name: 'Latch vs Flip-Flop',        view: 'waveform',  time: '—',              space: '—' },
    { id: 'jk_flipflop',             name: 'JK Flip-Flop',              view: 'schematic', time: 'edge-triggered', space: '2 latches' },
    { id: 't_flipflop_divider',      name: 'T as a Frequency Divider',  view: 'waveform',  time: 'edge-triggered', space: '3 T-FF' },
    { id: 'setup_hold',              name: 'Setup and Hold',            view: 'waveform',  time: 'edge-triggered', space: '1 D-FF' },
    { id: 'metastability',           name: 'Metastability',             view: 'waveform',  time: 'edge-triggered', space: '1 D-FF' },
  ]},
  { chapter: '5 · Counters', items: [
    { id: 'ripple_counter', name: 'Asynchronous Ripple Counter', view: 'schematic', time: 'O(n) delay',    space: '4 T-FF' },
    { id: 'sync_counter',   name: 'Synchronous Counter',         view: 'schematic', time: 'O(1) per stage', space: '4 T-FF + AND' },
    { id: 'mod_n_counter',  name: 'Mod-N Counter',                view: 'schematic', time: 'O(1) per stage', space: '4 T-FF + decoder' },
    { id: 'ring_johnson',   name: 'Ring and Johnson',             view: 'array',     time: 'O(1) per step',  space: '4 flip-flops' },
    { id: 'gray_counter',   name: 'Gray Code',                    view: 'array',     time: 'O(1) per step',  space: 'XOR + shift' },
  ]},
  { chapter: '6 · FSM', items: [
    { id: 'moore_vs_mealy',      name: 'Moore vs Mealy',              view: 'graph', time: 'O(1) per symbol', space: '2 states' },
    { id: 'seq_detector_101',    name: "'101' Sequence Detector (Moore)", view: 'graph', time: 'O(n)',        space: '4 states' },
    { id: 'moore_mealy_timing',  name: 'Same Input, Different Outputs', view: 'waveform', time: 'O(1) per symbol', space: '2 states' },
    { id: 'traffic_light',       name: 'Traffic Light',               view: 'graph', time: 'O(1) per tick',   space: '3 states' },
  ]},
];
