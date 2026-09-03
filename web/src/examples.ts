import type {
  GraphEdge,
  GraphRequest,
  GraphVertex
} from './types';

export type GraphExampleCategory =
  | 'planar'
  | 'non-planar'
  | 'subdivision';

export interface GraphExample {
  id: string;
  name: string;
  description: string;
  category: GraphExampleCategory;
  expectedPlanar: boolean;
  graph: GraphRequest;
}

function buildGraph(
  labels: string[],
  edgePairs: Array<[number, number]>
): GraphRequest {
  const vertices: GraphVertex[] =
    labels.map((label, index) => ({
      id: `v${index + 1}`,
      label
    }));

  const edges: GraphEdge[] =
    edgePairs.map(
      ([sourceIndex, targetIndex], index) => ({
        id: `e${index + 1}`,
        source: vertices[sourceIndex].id,
        target: vertices[targetIndex].id
      })
    );

  return {
    vertices,
    edges
  };
}

function completeGraph(
  labels: string[]
): GraphRequest {
  const edgePairs:
    Array<[number, number]> = [];

  for (
    let first = 0;
    first < labels.length;
    ++first
  ) {
    for (
      let second = first + 1;
      second < labels.length;
      ++second
    ) {
      edgePairs.push([
        first,
        second
      ]);
    }
  }

  return buildGraph(
    labels,
    edgePairs
  );
}

const triangleExample =
  buildGraph(
    ['A', 'B', 'C'],
    [
      [0, 1],
      [1, 2],
      [2, 0]
    ]
  );

const k4Example =
  completeGraph([
    'A',
    'B',
    'C',
    'D'
  ]);

const cubeExample =
  buildGraph(
    [
      'A', 'B', 'C', 'D',
      'E', 'F', 'G', 'H'
    ],
    [
      [0, 1], [1, 2], [2, 3], [3, 0],
      [4, 5], [5, 6], [6, 7], [7, 4],
      [0, 4], [1, 5], [2, 6], [3, 7]
    ]
  );

const k33Example =
  buildGraph(
    [
      'A1', 'A2', 'A3',
      'B1', 'B2', 'B3'
    ],
    [
      [0, 3], [0, 4], [0, 5],
      [1, 3], [1, 4], [1, 5],
      [2, 3], [2, 4], [2, 5]
    ]
  );

const k5Example =
  completeGraph([
    'A',
    'B',
    'C',
    'D',
    'E'
  ]);

const petersenExample =
  buildGraph(
    [
      'A', 'B', 'C', 'D', 'E',
      'A′', 'B′', 'C′', 'D′', 'E′'
    ],
    [
      [0, 1], [1, 2], [2, 3], [3, 4], [4, 0],
      [5, 7], [7, 9], [9, 6], [6, 8], [8, 5],
      [0, 5], [1, 6], [2, 7], [3, 8], [4, 9]
    ]
  );

const subdividedK5Example =
  buildGraph(
    [
      'A',
      'B',
      'C',
      'D',
      'E',
      'S'
    ],
    [
      [0, 5], [5, 1],
      [0, 2], [0, 3], [0, 4],
      [1, 2], [1, 3], [1, 4],
      [2, 3], [2, 4],
      [3, 4]
    ]
  );

export const graphExamples:
  GraphExample[] = [
    {
      id: 'triangle',
      name: 'Trougao C₃',
      description:
        'Najjednostavniji ciklus i osnovni planarni primjer.',
      category: 'planar',
      expectedPlanar: true,
      graph: triangleExample
    },
    {
      id: 'k4',
      name: 'Kompletni graf K₄',
      description:
        'Kompletan graf sa četiri čvora koji je i dalje planaran.',
      category: 'planar',
      expectedPlanar: true,
      graph: k4Example
    },
    {
      id: 'cube',
      name: 'Graf kocke Q₃',
      description:
        'Planarni 3-regularni graf sa osam čvorova.',
      category: 'planar',
      expectedPlanar: true,
      graph: cubeExample
    },
    {
      id: 'k33',
      name: 'Kompletni bipartitni graf K₃,₃',
      description:
        'Klasična Kuratowskijeva prepreka planarnosti.',
      category: 'non-planar',
      expectedPlanar: false,
      graph: k33Example
    },
    {
      id: 'k5',
      name: 'Kompletni graf K₅',
      description:
        'Druga osnovna Kuratowskijeva prepreka planarnosti.',
      category: 'non-planar',
      expectedPlanar: false,
      graph: k5Example
    },
    {
      id: 'petersen',
      name: 'Petersenov graf',
      description:
        'Poznat 3-regularni neplanarni graf koji nije samo K₅ ili K₃,₃.',
      category: 'non-planar',
      expectedPlanar: false,
      graph: petersenExample
    },
    {
      id: 'subdivided-k5',
      name: 'Subdivizija K₅',
      description:
        'Jedna grana grafa K₅ zamijenjena je putem preko dodatnog čvora.',
      category: 'subdivision',
      expectedPlanar: false,
      graph: subdividedK5Example
    }
  ];
