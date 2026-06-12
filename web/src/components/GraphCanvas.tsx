import {
  useEffect,
  useRef,
  forwardRef,
  useImperativeHandle
} from 'react';

import cytoscape, {
  type Core,
  type ElementDefinition
} from 'cytoscape';

import type {
  GraphEdge,
  GraphVertex,
  KuratowskiCertificate,
  GraphPositions
} from '../types';

export interface GraphCanvasHandle {
  exportPng: () => string | null;
  exportSvg: () => string | null;
}

const EMPTY_SUBDIVISION_VERTEX_IDS:
  string[] = [];

function escapeXml(
  value: string
): string {
  return value
    .replaceAll('&', '&amp;')
    .replaceAll('<', '&lt;')
    .replaceAll('>', '&gt;')
    .replaceAll('"', '&quot;')
    .replaceAll("'", '&apos;');
}

interface GraphCanvasProps {
  vertices: GraphVertex[];
  edges: GraphEdge[];

  selectedNodeId: string | null;
  pendingEdgeSourceId: string | null;

  certificate?: KuratowskiCertificate;

  positionsByVertex?: GraphPositions;
  subdivisionVertexIds?: string[];

  onNodeTap: (
    nodeId: string
  ) => void;

  onCanvasTap: () => void;

  onPositionsChange: (
    positions: GraphPositions
  ) => void;
}

function buildNodeClasses(
  vertexId: string,
  selectedNodeId: string | null,
  pendingEdgeSourceId: string | null,
  subdivisionVertexIds: string[],
  certificate?: KuratowskiCertificate,
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
    subdivisionVertexIds.includes(vertexId)
  ) {
    classes.push('subdivision');
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

function collectPositions(
  cy: Core
): GraphPositions {
  const positions:
    GraphPositions = {};

  cy.nodes().forEach(node => {
    const position =
      node.position();

    positions[node.id()] = {
      x: position.x,
      y: position.y
    };
  });

  return positions;
}

export const GraphCanvas = forwardRef<
  GraphCanvasHandle,
  GraphCanvasProps
>(function GraphCanvas({
  vertices,
  edges,
  selectedNodeId,
  pendingEdgeSourceId,
  certificate,
  positionsByVertex,
  subdivisionVertexIds =
    EMPTY_SUBDIVISION_VERTEX_IDS,
  onNodeTap,
  onCanvasTap,
  onPositionsChange
}: GraphCanvasProps, ref) {
  const containerRef =
    useRef<HTMLDivElement | null>(
      null
    );

  const cytoscapeRef =
    useRef<Core | null>(
      null
    );

  const viewportRef =
    useRef<{
      zoom: number;
      pan: {
        x: number;
        y: number;
      };
    } | null>(null);

  useImperativeHandle(
    ref,
    () => ({
      exportPng: () => {
        const cy =
          cytoscapeRef.current;

        if (!cy) {
          return null;
        }

        return cy.png({
          full: true,
          scale: 2,
          bg: '#ffffff'
        });
      },

      exportSvg: () => {
        const cy =
          cytoscapeRef.current;

        if (!cy) {
          return null;
        }

        const boundingBox =
          cy.elements().boundingBox();

        const padding = 48;

        const width =
          Math.max(
            400,
            boundingBox.w
              + 2 * padding
          );

        const height =
          Math.max(
            300,
            boundingBox.h
              + 2 * padding
          );

        const offsetX =
          padding
          - boundingBox.x1;

        const offsetY =
          padding
          - boundingBox.y1;

        const edgeSvg =
          cy.edges()
            .map(edge => {
              const source =
                edge.source()
                  .position();

              const target =
                edge.target()
                  .position();

              const certificateEdge =
                edge.hasClass(
                  'certificate'
                );

              return `
                <line
                  x1="${source.x + offsetX}"
                  y1="${source.y + offsetY}"
                  x2="${target.x + offsetX}"
                  y2="${target.y + offsetY}"
                  stroke="${
                    certificateEdge
                      ? '#dc2626'
                      : '#94a3b8'
                  }"
                  stroke-width="${
                    certificateEdge
                      ? 7
                      : 3
                  }"
                  stroke-linecap="round"
                />
              `;
            })
            .join('');

        const nodeSvg =
          cy.nodes()
            .map(node => {
              const position =
                node.position();

              const certificateNode =
                node.hasClass(
                  'certificate'
                );

              const subdivisionNode =
                node.hasClass(
                  'subdivision'
                );

              const fill =
                certificateNode
                  ? '#dc2626'
                  : subdivisionNode
                    ? '#f97316'
                    : '#334155';

              const label =
                escapeXml(
                  String(
                    node.data('label')
                  )
                );

              return `
                <g>
                  <circle
                    cx="${position.x + offsetX}"
                    cy="${position.y + offsetY}"
                    r="23"
                    fill="${fill}"
                    stroke="#0f172a"
                    stroke-width="2"
                  />

                  <text
                    x="${position.x + offsetX}"
                    y="${position.y + offsetY + 43}"
                    text-anchor="middle"
                    font-family="Arial, sans-serif"
                    font-size="14"
                    font-weight="700"
                    fill="#0f172a"
                  >${label}</text>
                </g>
              `;
            })
            .join('');

        return `
          <svg
            xmlns="http://www.w3.org/2000/svg"
            width="${width}"
            height="${height}"
            viewBox="0 0 ${width} ${height}"
          >
            <rect
              width="100%"
              height="100%"
              fill="#ffffff"
            />

            ${edgeSvg}

            ${nodeSvg}
          </svg>
        `;
      }
    }),
    []
  );

  useEffect(() => {
    if (!containerRef.current) {
      return;
    }

    const hasPresetPositions =
      vertices.length > 0
      && vertices.every(vertex =>
        positionsByVertex?.[
          vertex.id
        ] !== undefined
      );

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
              subdivisionVertexIds,
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

    const previousCy =
      cytoscapeRef.current;

    if (previousCy) {
      viewportRef.current = {
        zoom:
          previousCy.zoom(),

        pan:
          previousCy.pan()
      };
    }

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
            'node.subdivision',

          style: {
            'background-color':
              '#f97316',

            'border-color':
              '#c2410c',

            'border-width': 4
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
          hasPresetPositions
            ? 'preset'
            : 'cose',

        animate: false,

        fit:
          !hasPresetPositions,

        padding: 36
      },

      minZoom: 0.3,
      maxZoom: 3
    });

    if (
      hasPresetPositions
      && viewportRef.current
    ) {
      cy.zoom(
        viewportRef.current.zoom
      );

      cy.pan(
        viewportRef.current.pan
      );
    }

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

    cy.on(
      'dragfree',
      'node',
      () => {
        onPositionsChange(
          collectPositions(cy)
        );
      }
    );

    if (!hasPresetPositions) {
      onPositionsChange(
        collectPositions(cy)
      );
    }

    cytoscapeRef.current = cy;

    return () => {
      viewportRef.current = {
        zoom:
          cy.zoom(),

        pan:
          cy.pan()
      };

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
    subdivisionVertexIds,
    onNodeTap,
    onCanvasTap,
    onPositionsChange,
  ]);

  return (
    <div
      ref={containerRef}
      className="graph-canvas"
    />
  );
});