import type {
  GraphRequest
} from './types';

export const triangleExample: GraphRequest = {
  vertices: [
    {
      id: 'v1',
      label: 'A'
    },
    {
      id: 'v2',
      label: 'B'
    },
    {
      id: 'v3',
      label: 'C'
    }
  ],

  edges: [
    {
      id: 'e1',
      source: 'v1',
      target: 'v2'
    },
    {
      id: 'e2',
      source: 'v2',
      target: 'v3'
    },
    {
      id: 'e3',
      source: 'v3',
      target: 'v1'
    }
  ]
};

export const k33Example: GraphRequest = {
  vertices: [
    {
      id: 'a1',
      label: 'A1'
    },
    {
      id: 'a2',
      label: 'A2'
    },
    {
      id: 'a3',
      label: 'A3'
    },
    {
      id: 'b1',
      label: 'B1'
    },
    {
      id: 'b2',
      label: 'B2'
    },
    {
      id: 'b3',
      label: 'B3'
    }
  ],

  edges: [
    {
      id: 'e1',
      source: 'a1',
      target: 'b1'
    },
    {
      id: 'e2',
      source: 'a1',
      target: 'b2'
    },
    {
      id: 'e3',
      source: 'a1',
      target: 'b3'
    },

    {
      id: 'e4',
      source: 'a2',
      target: 'b1'
    },
    {
      id: 'e5',
      source: 'a2',
      target: 'b2'
    },
    {
      id: 'e6',
      source: 'a2',
      target: 'b3'
    },

    {
      id: 'e7',
      source: 'a3',
      target: 'b1'
    },
    {
      id: 'e8',
      source: 'a3',
      target: 'b2'
    },
    {
      id: 'e9',
      source: 'a3',
      target: 'b3'
    }
  ]
};