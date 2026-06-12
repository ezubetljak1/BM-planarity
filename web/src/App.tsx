import {
  useCallback,
  useMemo,
  useRef,
  useState
} from 'react';

import './App.css';

import {
  analyzeGraph
} from './api/planarityApi';

import {
  GraphCanvas,
  type GraphCanvasHandle
} from './components/GraphCanvas';

import {
  GraphIoPanel
} from './components/GraphIoPanel';

import type {
  AnalysisSuccessResponse,
  GraphEdge,
  GraphPositions,
  GraphRequest,
  GraphVertex,
  KuratowskiCertificate
} from './types';

function downloadTextFile(
  filename: string,
  content: string,
  type: string
) {
  const blob =
    new Blob(
      [content],
      { type }
    );

  const url =
    URL.createObjectURL(
      blob
    );

  const anchor =
    document.createElement(
      'a'
    );

  anchor.href = url;
  anchor.download = filename;

  document.body.appendChild(
    anchor
  );

  anchor.click();

  document.body.removeChild(
    anchor
  );

  URL.revokeObjectURL(
    url
  );
}

function createCircularPositions(
  vertices: GraphVertex[]
): GraphPositions {
  const positions:
    GraphPositions = {};

  const count =
    Math.max(
      1,
      vertices.length
    );

  vertices.forEach(
    (
      vertex,
      index
    ) => {
      const angle =
        2
        * Math.PI
        * index
        / count;

      positions[vertex.id] = {
        x:
          360
          + 220
          * Math.cos(angle),

        y:
          280
          + 220
          * Math.sin(angle)
      };
    }
  );

  return positions;
}

function canonicalEdgeKey(
  firstVertexId: string,
  secondVertexId: string
) {
  return firstVertexId
    < secondVertexId
      ? `${firstVertexId}|${secondVertexId}`
      : `${secondVertexId}|${firstVertexId}`;
}

function findSubdivisionPaths(
  edges: GraphEdge[],
  certificate:
    KuratowskiCertificate
    | undefined
): string[][] {
  if (!certificate) {
    return [];
  }

  const certificateEdgeIds =
    new Set(
      certificate.edgeIds
    );

  const branchVertexIds =
    new Set(
      certificate
        .branchVertexIds
    );

  const adjacency =
    new Map<
      string,
      string[]
    >();

  function addNeighbor(
    vertexId: string,
    neighborId: string
  ) {
    const neighbors =
      adjacency.get(vertexId)
      ?? [];

    neighbors.push(
      neighborId
    );

    adjacency.set(
      vertexId,
      neighbors
    );
  }

  edges.forEach(edge => {
    if (
      !certificateEdgeIds.has(
        edge.id
      )
    ) {
      return;
    }

    addNeighbor(
      edge.source,
      edge.target
    );

    addNeighbor(
      edge.target,
      edge.source
    );
  });

  const visitedEdges =
    new Set<string>();

  const subdivisionPaths:
    string[][] = [];

  branchVertexIds.forEach(
    branchVertexId => {
      const neighbors =
        adjacency.get(
          branchVertexId
        )
        ?? [];

      neighbors.forEach(
        neighborId => {
          const firstEdgeKey =
            canonicalEdgeKey(
              branchVertexId,
              neighborId
            );

          if (
            visitedEdges.has(
              firstEdgeKey
            )
          ) {
            return;
          }

          const path = [
            branchVertexId
          ];

          let previousVertexId =
            branchVertexId;

          let currentVertexId =
            neighborId;

          visitedEdges.add(
            firstEdgeKey
          );

          while (true) {
            path.push(
              currentVertexId
            );

            if (
              branchVertexIds.has(
                currentVertexId
              )
            ) {
              break;
            }

            const nextCandidates =
              (
                adjacency.get(
                  currentVertexId
                )
                ?? []
              ).filter(
                candidate =>
                  candidate
                  !== previousVertexId
              );

            if (
              nextCandidates.length
              !== 1
            ) {
              break;
            }

            const nextVertexId =
              nextCandidates[0];

            visitedEdges.add(
              canonicalEdgeKey(
                currentVertexId,
                nextVertexId
              )
            );

            previousVertexId =
              currentVertexId;

            currentVertexId =
              nextVertexId;
          }

          if (path.length > 2) {
            subdivisionPaths.push(
              path
            );
          }
        }
      );
    }
  );

  return subdivisionPaths;
}

function App() {
  const [vertices, setVertices] =
    useState<GraphVertex[]>([]);

  const [edges, setEdges] =
    useState<GraphEdge[]>([]);

  const [
    positionsByVertex,
    setPositionsByVertex
  ] = useState<GraphPositions>(
    {}
  );

  const [
    newVertexLabel,
    setNewVertexLabel
  ] = useState('');

  const [
    edgeSource,
    setEdgeSource
  ] = useState('');

  const [
    edgeTarget,
    setEdgeTarget
  ] = useState('');

  const [
    selectedNodeId,
    setSelectedNodeId
  ] = useState<
    string | null
  >(null);

  const [
    isEdgeClickMode,
    setIsEdgeClickMode
  ] = useState(false);

  const [
    pendingEdgeSourceId,
    setPendingEdgeSourceId
  ] = useState<
    string | null
  >(null);

  const [
    analysis,
    setAnalysis
  ] =
    useState<
      AnalysisSuccessResponse
      | null
    >(null);

  const [error, setError] =
    useState('');

  const [
    isAnalyzing,
    setIsAnalyzing
  ] = useState(false);

  const nextVertexId =
    useRef(1);

  const nextEdgeId =
    useRef(1);

  const graphCanvasRef =
    useRef<
      GraphCanvasHandle
      | null
    >(null);

  const certificate =
    analysis?.planar
      ? undefined
      : analysis
        ?.certificate;

  const selectedNode =
    useMemo(
      () =>
        vertices.find(
          vertex =>
            vertex.id
            === selectedNodeId
        )
        ?? null,
      [
        vertices,
        selectedNodeId
      ]
    );

  const subdivisionPaths =
    useMemo(
      () =>
        findSubdivisionPaths(
          edges,
          certificate
        ),
      [
        edges,
        certificate
      ]
    );

  const subdivisionVertexIds =
    useMemo(
      () =>
        [
          ...new Set(
            subdivisionPaths
              .flatMap(
                path =>
                  path.slice(
                    1,
                    -1
                  )
              )
          )
        ],
      [
        subdivisionPaths
      ]
    );

  const handlePositionsChange =
    useCallback(
      (
        nextPositions:
          GraphPositions
      ) => {
        setPositionsByVertex(
          nextPositions
        );
      },
      []
    );

  function vertexLabel(
    vertexId: string
  ) {
    return (
      vertices.find(
        vertex =>
          vertex.id === vertexId
      )
      ?.label
      ?? vertexId
    );
  }

  function edgeNeighborLabel(
    vertexId: string,
    edgeId: string
  ) {
    const edge =
      edges.find(
        candidate =>
          candidate.id === edgeId
      );

    if (!edge) {
      return edgeId;
    }

    const neighborId =
      edge.source === vertexId
        ? edge.target
        : edge.source;

    return vertexLabel(
      neighborId
    );
  }

  function clearAnalysis() {
    setAnalysis(null);
    setError('');
  }

  function createNextVertexId() {
    let candidate = '';

    do {
      candidate =
        `v${nextVertexId.current++}`;
    } while (
      vertices.some(
        vertex =>
          vertex.id
          === candidate
      )
    );

    return candidate;
  }

  function createNextEdgeId() {
    let candidate = '';

    do {
      candidate =
        `e${nextEdgeId.current++}`;
    } while (
      edges.some(
        edge =>
          edge.id
          === candidate
      )
    );

    return candidate;
  }

  function addVertex() {
    const label =
      newVertexLabel.trim();

    if (!label) {
      setError(
        'Unesite oznaku čvora.'
      );

      return;
    }

    const vertex:
      GraphVertex = {
        id:
          createNextVertexId(),

        label
      };

    setVertices(current => [
      ...current,
      vertex
    ]);

    setPositionsByVertex(
      current => ({
        ...current,

        [vertex.id]: {
          x:
            140
            + vertices.length
            * 45,

          y:
            140
            + vertices.length
            * 25
        }
      })
    );

    setNewVertexLabel('');

    setError('');
    setAnalysis(null);
  }

  function appendEdge(
    source: string,
    target: string
  ): boolean {
    if (!source || !target) {
      setError(
        'Izaberite oba kraja grane.'
      );

      return false;
    }

    if (source === target) {
      setError(
        'Self-loop grane nisu podržane.'
      );

      return false;
    }

    const duplicateExists =
      edges.some(edge =>
        (
          edge.source === source
          && edge.target === target
        )
        || (
          edge.source === target
          && edge.target === source
        )
      );

    if (duplicateExists) {
      setError(
        'Paralelna grana već postoji.'
      );

      return false;
    }

    const edge:
      GraphEdge = {
        id:
          createNextEdgeId(),

        source,
        target
      };

    setEdges(current => [
      ...current,
      edge
    ]);

    setError('');
    setAnalysis(null);

    return true;
  }

  function addEdge() {
    if (
      appendEdge(
        edgeSource,
        edgeTarget
      )
    ) {
      setEdgeSource('');
      setEdgeTarget('');
    }
  }

  function handleCanvasNodeTap(
    nodeId: string
  ) {
    if (!isEdgeClickMode) {
      setSelectedNodeId(
        nodeId
      );

      return;
    }

    if (!pendingEdgeSourceId) {
      setPendingEdgeSourceId(
        nodeId
      );

      setSelectedNodeId(
        nodeId
      );

      setError('');

      return;
    }

    if (
      appendEdge(
        pendingEdgeSourceId,
        nodeId
      )
    ) {
      setPendingEdgeSourceId(
        null
      );

      setSelectedNodeId(
        null
      );
    }
  }

  function handleCanvasTap() {
    setSelectedNodeId(
      null
    );

    if (isEdgeClickMode) {
      setPendingEdgeSourceId(
        null
      );
    }
  }

  function toggleEdgeClickMode() {
    setIsEdgeClickMode(
      current => !current
    );

    setPendingEdgeSourceId(
      null
    );

    setSelectedNodeId(
      null
    );

    setError('');
  }

  function removeVertex(
    vertexId: string
  ) {
    setVertices(current =>
      current.filter(
        vertex =>
          vertex.id
          !== vertexId
      )
    );

    setEdges(current =>
      current.filter(
        edge =>
          edge.source
          !== vertexId
          && edge.target
          !== vertexId
      )
    );

    setPositionsByVertex(
      current => {
        const nextPositions = {
          ...current
        };

        delete nextPositions[
          vertexId
        ];

        return nextPositions;
      }
    );

    if (
      edgeSource === vertexId
    ) {
      setEdgeSource('');
    }

    if (
      edgeTarget === vertexId
    ) {
      setEdgeTarget('');
    }

    if (
      selectedNodeId
      === vertexId
    ) {
      setSelectedNodeId(
        null
      );
    }

    if (
      pendingEdgeSourceId
      === vertexId
    ) {
      setPendingEdgeSourceId(
        null
      );
    }

    clearAnalysis();
  }

  function removeEdge(
    edgeId: string
  ) {
    setEdges(current =>
      current.filter(
        edge =>
          edge.id !== edgeId
      )
    );

    clearAnalysis();
  }

  function loadGraph(
    graph: GraphRequest
  ) {
    const nextVertices =
      graph.vertices.map(
        vertex => ({
          ...vertex
        })
      );

    const nextEdges =
      graph.edges.map(
        edge => ({
          ...edge
        })
      );

    setVertices(
      nextVertices
    );

    setEdges(
      nextEdges
    );

    setPositionsByVertex(
      createCircularPositions(
        nextVertices
      )
    );

    setEdgeSource('');
    setEdgeTarget('');

    setSelectedNodeId(null);

    setIsEdgeClickMode(
      false
    );

    setPendingEdgeSourceId(
      null
    );

    setAnalysis(null);
    setError('');

    nextVertexId.current = 1;
    nextEdgeId.current = 1;
  }

  function resetGraph() {
    setVertices([]);
    setEdges([]);

    setPositionsByVertex(
      {}
    );

    setEdgeSource('');
    setEdgeTarget('');

    setSelectedNodeId(null);

    setIsEdgeClickMode(
      false
    );

    setPendingEdgeSourceId(
      null
    );

    setAnalysis(null);
    setError('');

    nextVertexId.current = 1;
    nextEdgeId.current = 1;
  }

  function exportPng() {
    const dataUrl =
      graphCanvasRef
        .current
        ?.exportPng();

    if (!dataUrl) {
      setError(
        'Nije moguće izvesti PNG.'
      );

      return;
    }

    const anchor =
      document.createElement(
        'a'
      );

    anchor.href = dataUrl;

    anchor.download =
      'graph-visualization.png';

    document.body.appendChild(
      anchor
    );

    anchor.click();

    document.body.removeChild(
      anchor
    );
  }

  function exportSvg() {
    const svg =
      graphCanvasRef
        .current
        ?.exportSvg();

    if (!svg) {
      setError(
        'Nije moguće izvesti SVG.'
      );

      return;
    }

    downloadTextFile(
      'graph-visualization.svg',
      svg,
      'image/svg+xml'
    );
  }

  async function runAnalysis() {
    setIsAnalyzing(true);
    setError('');

    try {
      const result =
        await analyzeGraph({
          vertices,
          edges
        });

      if (
        result.planar
        && result.layout
          ?.positionsByVertex
      ) {
        setPositionsByVertex(
          result.layout
            .positionsByVertex
        );
      }

      setAnalysis(result);
    } catch (caughtError) {
      const message =
        caughtError
        instanceof Error
          ? caughtError.message
          : 'Nepoznata greška.';

      setError(message);
      setAnalysis(null);
    } finally {
      setIsAnalyzing(false);
    }
  }

  return (
    <main className="app-shell">
      <header className="app-header">
        <div>
          <p className="eyebrow">
            Boyer–Myrvold planarity
          </p>

          <h1>
            Interaktivni test
            planarnosti
          </h1>

          <p className="intro">
            Kreirajte graf,
            pokrenite algoritam i
            pregledajte planar
            embedding ili Kuratowski
            certifikat.
          </p>
        </div>

        <button
          className="secondary-button"
          type="button"
          onClick={resetGraph}
        >
          Novi graf
        </button>
      </header>

      <section className="workspace">
        <aside className="control-panel">
          <GraphIoPanel
            graph={{
              vertices,
              edges
            }}
            analysis={analysis}
            onLoadGraph={loadGraph}
            onError={setError}
            onExportPng={exportPng}
            onExportSvg={exportSvg}
          />

          <section className="panel-section">
            <h2>
              Dodavanje čvora
            </h2>

            <div className="inline-form">
              <input
                value={newVertexLabel}
                placeholder="Oznaka, npr. A"
                onChange={event =>
                  setNewVertexLabel(
                    event.target.value
                  )
                }
                onKeyDown={event => {
                  if (
                    event.key
                    === 'Enter'
                  ) {
                    addVertex();
                  }
                }}
              />

              <button
                type="button"
                onClick={addVertex}
              >
                Dodaj
              </button>
            </div>
          </section>

          <section className="panel-section">
            <h2>
              Dodavanje grane
            </h2>

            <select
              value={edgeSource}
              onChange={event =>
                setEdgeSource(
                  event.target.value
                )
              }
            >
              <option value="">
                Početni čvor
              </option>

              {vertices.map(vertex => (
                <option
                  key={vertex.id}
                  value={vertex.id}
                >
                  {vertex.label}
                </option>
              ))}
            </select>

            <select
              value={edgeTarget}
              onChange={event =>
                setEdgeTarget(
                  event.target.value
                )
              }
            >
              <option value="">
                Krajnji čvor
              </option>

              {vertices.map(vertex => (
                <option
                  key={vertex.id}
                  value={vertex.id}
                >
                  {vertex.label}
                </option>
              ))}
            </select>

            <button
              type="button"
              onClick={addEdge}
            >
              Dodaj granu
            </button>

            <button
              className={
                isEdgeClickMode
                  ? 'edge-mode-button active'
                  : 'secondary-button edge-mode-button'
              }
              type="button"
              onClick={
                toggleEdgeClickMode
              }
            >
              {
                isEdgeClickMode
                  ? 'Završi dodavanje klikom'
                  : 'Dodaj granu klikom'
              }
            </button>

            {
              isEdgeClickMode
              && (
                <p className="edge-mode-help">
                  {
                    pendingEdgeSourceId
                      ? 'Kliknite drugi čvor.'
                      : 'Kliknite prvi čvor.'
                  }
                </p>
              )
            }
          </section>

          <section className="panel-section">
            <h2>
              Čvorovi
            </h2>

            {
              vertices.length === 0
                ? (
                  <p className="empty-text">
                    Nema čvorova.
                  </p>
                )
                : (
                  <ul className="item-list">
                    {
                      vertices.map(
                        vertex => (
                          <li key={vertex.id}>
                            <span>
                              {vertex.label}
                            </span>

                            <button
                              className="danger-link"
                              type="button"
                              onClick={() =>
                                removeVertex(
                                  vertex.id
                                )
                              }
                            >
                              Obriši
                            </button>
                          </li>
                        )
                      )
                    }
                  </ul>
                )
            }
          </section>

          <section className="panel-section">
            <h2>
              Grane
            </h2>

            {
              edges.length === 0
                ? (
                  <p className="empty-text">
                    Nema grana.
                  </p>
                )
                : (
                  <ul className="item-list">
                    {
                      edges.map(edge => (
                        <li key={edge.id}>
                          <span>
                            {
                              vertexLabel(
                                edge.source
                              )
                            }
                            {' — '}
                            {
                              vertexLabel(
                                edge.target
                              )
                            }
                          </span>

                          <button
                            className="danger-link"
                            type="button"
                            onClick={() =>
                              removeEdge(
                                edge.id
                              )
                            }
                          >
                            Obriši
                          </button>
                        </li>
                      ))
                    }
                  </ul>
                )
            }
          </section>
        </aside>

        <section className="canvas-panel">
          <GraphCanvas
            ref={graphCanvasRef}
            vertices={vertices}
            edges={edges}
            selectedNodeId={
              selectedNodeId
            }
            pendingEdgeSourceId={
              pendingEdgeSourceId
            }
            certificate={
              certificate
            }
            positionsByVertex={
              positionsByVertex
            }
            subdivisionVertexIds={
              subdivisionVertexIds
            }
            onNodeTap={
              handleCanvasNodeTap
            }
            onCanvasTap={
              handleCanvasTap
            }
            onPositionsChange={
              handlePositionsChange
            }
          />

          <div className="canvas-footer">
            <span>
              Čvorovi: {
                vertices.length
              }
            </span>

            <span>
              Grane: {
                edges.length
              }
            </span>

            {
              selectedNode
              && (
                <span>
                  Odabrano: {
                    selectedNode.label
                  }
                </span>
              )
            }
          </div>
        </section>

        <aside className="result-panel">
          <button
            className="analyze-button"
            type="button"
            disabled={isAnalyzing}
            onClick={runAnalysis}
          >
            {
              isAnalyzing
                ? 'Analiza...'
                : 'Pokreni test planarnosti'
            }
          </button>

          {
            error
            && (
              <div className="error-card">
                {error}
              </div>
            )
          }

          {
            !analysis
            && !error
            && (
              <p className="empty-text">
                Rezultat će se prikazati
                nakon pokretanja
                algoritma.
              </p>
            )
          }

          {
            analysis?.planar
            && (
              <section className="result-card planar">
                <h2>
                  Graf je planaran
                </h2>

                <p>
                  Pronađen je validan
                  planar embedding.
                </p>

                <h3>
                  Clockwise rotacije
                </h3>

                <p className="muted-text">
                  Za svaki čvor prikazan
                  je clockwise poredak
                  njegovih susjednih
                  čvorova.
                </p>

                <ul className="rotation-list">
                  {
                    Object.entries(
                      analysis
                        .embedding
                        ?.clockwiseEdgesAroundVertex
                      ?? {}
                    ).map(
                      (
                        [
                          vertexId,
                          edgeIds
                        ]
                      ) => (
                        <li key={vertexId}>
                          <strong>
                            {
                              vertexLabel(
                                vertexId
                              )
                            }
                          </strong>

                          <span>
                            {
                              edgeIds.length
                                ? edgeIds
                                  .map(
                                    edgeId =>
                                      edgeNeighborLabel(
                                        vertexId,
                                        edgeId
                                      )
                                  )
                                  .join(', ')
                                : '—'
                            }
                          </span>
                        </li>
                      )
                    )
                  }
                </ul>
              </section>
            )
          }

          {
            analysis
            && !analysis.planar
            && analysis.certificate
            && (
              <section className="result-card non-planar">
                <h2>
                  Graf nije planaran
                </h2>

                <p>
                  Pronađen je Kuratowski
                  certifikat:
                </p>

                <div className="certificate-type">
                  {
                    analysis
                      .certificate
                      .type
                  }
                </div>

                <ul className="legend-list">
                  <li>
                    <span className="legend-dot branch" />

                    Branch čvor
                  </li>

                  <li>
                    <span className="legend-dot subdivision" />

                    Subdivision čvor
                  </li>

                  <li>
                    <span className="legend-line certificate" />

                    Grana certifikata
                  </li>
                </ul>

                <h3>
                  Branch čvorovi
                </h3>

                <ul>
                  {
                    analysis
                      .certificate
                      .branchVertexIds
                      .map(
                        vertexId => (
                          <li key={vertexId}>
                            {
                              vertexLabel(
                                vertexId
                              )
                            }
                          </li>
                        )
                      )
                  }
                </ul>

                {
                  subdivisionPaths.length
                  > 0
                  && (
                    <>
                      <h3>
                        Subdivision putevi
                      </h3>

                      <ul className="subdivision-path-list">
                        {
                          subdivisionPaths.map(
                            (
                              path,
                              index
                            ) => (
                              <li key={index}>
                                {
                                  path
                                    .map(
                                      vertexId =>
                                        vertexLabel(
                                          vertexId
                                        )
                                    )
                                    .join(' — ')
                                }
                              </li>
                            )
                          )
                        }
                      </ul>

                      <p className="muted-text">
                        Narandžasti čvorovi
                        nalaze se unutar
                        subdivizijskih puteva.
                        Sažimanjem tih puteva
                        dobija se označeni
                        K3,3 ili K5 graf.
                      </p>
                    </>
                  )
                }
              </section>
            )
          }
        </aside>
      </section>
    </main>
  );
}

export default App;