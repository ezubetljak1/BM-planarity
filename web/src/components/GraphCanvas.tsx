import {
  useEffect,
  useRef
} from 'react';

import cytoscape, {
  type Core,
  type ElementDefinition
} from 'cytoscape';

import type {
  GraphEdge,
  GraphVertex,
  KuratowskiCertificate
} from '../types';

interface GraphCanvasProps {
  vertices: GraphVertex[];
  edges: GraphEdge[];

  selectedNodeId: string | null;
  pendingEdgeSourceId: string | null;

  certificate?: KuratowskiCertificate;

  positionsByVertex?: Record<
    string, 
    {
        x: number;
        y: number;
    }
  >;

  onNodeTap: (
    nodeId: string
  ) => void;

  onCanvasTap: () => void;
}

function buildNodeClasses(
  vertexId: string,
  selectedNodeId: string | null,
  pendingEdgeSourceId: string | null,
  certificate?: KuratowskiCertificate
): string {
  const classes: string[] = [];

  if (vertexId === selectedNodeId) {
    classes.push('selected');
  }

  if (
    vertexId === pendingEdgeSourceId
  ) {
    classes.push(
      'pending-edge-source'
    );
  }

  if (
    certificate
      ?.branchVertexIds
      .includes(vertexId)
  ) {
    classes.push('certificate');
  }

  return classes.join(' ');
}

function buildEdgeClasses(
  edgeId: string,
  certificate?: KuratowskiCertificate
): string {
  return certificate
    ?.edgeIds
    .includes(edgeId)
      ? 'certificate'
      : '';
}

export function GraphCanvas({
  vertices,
  edges,
  selectedNodeId,
  pendingEdgeSourceId,
  certificate,
  positionsByVertex,
  onNodeTap,
  onCanvasTap
}: GraphCanvasProps) {
  const containerRef =
    useRef<HTMLDivElement | null>(
      null
    );

  const cytoscapeRef =
    useRef<Core | null>(
      null
    );

  useEffect(() => {
    if (!containerRef.current) {
      return;
    }

    const elements:
      ElementDefinition[] = [
        ...vertices.map(vertex => ({
          data: {
            id: vertex.id,
            label: vertex.label
          },

          position:
            positionsByVertex?.[
                vertex.id
            ],

          classes:
            buildNodeClasses(
              vertex.id,
              selectedNodeId,
              pendingEdgeSourceId,
              certificate
            )
        })),

        ...edges.map(edge => ({
          data: {
            id: edge.id,
            source: edge.source,
            target: edge.target
          },

          classes:
            buildEdgeClasses(
              edge.id,
              certificate
            )
        }))
      ];

    cytoscapeRef.current
      ?.destroy();

    const cy = cytoscape({
      container:
        containerRef.current,

      elements,

      style: [
        {
          selector: 'node',

          style: {
            label: 'data(label)',

            width: 46,
            height: 46,

            'background-color':
              '#334155',

            color: '#0f172a',

            'font-size': 14,
            'font-weight': 600,

            'text-valign':
              'bottom',

            'text-margin-y': 8
          }
        },

        {
          selector: 'node.selected',

          style: {
            'background-color':
              '#2563eb',

            'border-color':
              '#1d4ed8',

            'border-width': 4
          }
        },

        {
          selector:
            'node.pending-edge-source',

          style: {
            'background-color':
              '#f59e0b',

            'border-color':
              '#b45309',

            'border-width': 5
          }
        },

        {
          selector:
            'node.certificate',

          style: {
            'background-color':
              '#dc2626',

            'border-color':
              '#991b1b',

            'border-width': 4
          }
        },

        {
          selector: 'edge',

          style: {
            width: 3,

            'line-color':
              '#94a3b8',

            'curve-style':
              'bezier'
          }
        },

        {
          selector:
            'edge.certificate',

          style: {
            width: 7,

            'line-color':
              '#dc2626'
          }
        }
      ],

      layout: {
        name: 
            positionsByVertex 
                ? 'preset'
                : 'cose',

        animate: false,
        fit: true,
        padding: 36
      },

      minZoom: 0.3,
      maxZoom: 3
    });

    cy.on(
      'tap',
      'node',
      event => {
        onNodeTap(
          event.target.id()
        );
      }
    );

    cy.on(
      'tap',
      event => {
        if (
          event.target === cy
        ) {
          onCanvasTap();
        }
      }
    );

    cytoscapeRef.current = cy;

    return () => {
      cy.destroy();

      if (
        cytoscapeRef.current
        === cy
      ) {
        cytoscapeRef.current =
          null;
      }
    };
  }, [
    vertices,
    edges,
    selectedNodeId,
    pendingEdgeSourceId,
    certificate,
    positionsByVertex,
    onNodeTap,
    onCanvasTap
  ]);

  return (
    <div
      ref={containerRef}
      className="graph-canvas"
    />
  );
}