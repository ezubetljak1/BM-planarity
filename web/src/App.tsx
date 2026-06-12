import {
  useMemo,
  useRef,
  useState
} from 'react';

import './App.css';

import {
  analyzeGraph
} from './api/planarityApi';

import {
  GraphCanvas
} from './components/GraphCanvas';

import type {
  AnalysisSuccessResponse,
  GraphEdge,
  GraphVertex
} from './types';

function App() {
  const [vertices, setVertices] =
    useState<GraphVertex[]>([]);

  const [edges, setEdges] =
    useState<GraphEdge[]>([]);

  const [newVertexLabel, setNewVertexLabel] =
    useState('');

  const [edgeSource, setEdgeSource] =
    useState('');

  const [edgeTarget, setEdgeTarget] =
    useState('');

  const [selectedNodeId, setSelectedNodeId] =
    useState<string | null>(null);

  const [analysis, setAnalysis] =
    useState<AnalysisSuccessResponse | null>(
      null
    );

  const [error, setError] =
    useState('');

  const [isAnalyzing, setIsAnalyzing] =
    useState(false);

  const nextVertexId =
    useRef(1);

  const nextEdgeId =
    useRef(1);

  const certificate =
    analysis?.planar
      ? undefined
      : analysis?.certificate;

  const selectedNode =
    useMemo(
      () =>
        vertices.find(
          vertex =>
            vertex.id === selectedNodeId
        ) ?? null,
      [
        vertices,
        selectedNodeId
      ]
    );

  function clearAnalysis() {
    setAnalysis(null);
    setError('');
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

    const vertex: GraphVertex = {
      id: `v${nextVertexId.current++}`,
      label
    };

    setVertices(current => [
      ...current,
      vertex
    ]);

    setNewVertexLabel('');
    setError('');
    setAnalysis(null);
  }

  function addEdge() {
    if (!edgeSource || !edgeTarget) {
      setError(
        'Izaberite oba kraja grane.'
      );

      return;
    }

    if (edgeSource === edgeTarget) {
      setError(
        'Self-loop grane nisu podržane.'
      );

      return;
    }

    const duplicateExists =
      edges.some(edge =>
        (
          edge.source === edgeSource
          && edge.target === edgeTarget
        )
        || (
          edge.source === edgeTarget
          && edge.target === edgeSource
        )
      );

    if (duplicateExists) {
      setError(
        'Paralelna grana već postoji.'
      );

      return;
    }

    const edge: GraphEdge = {
      id: `e${nextEdgeId.current++}`,
      source: edgeSource,
      target: edgeTarget
    };

    setEdges(current => [
      ...current,
      edge
    ]);

    setError('');
    setAnalysis(null);
  }

  function removeVertex(
    vertexId: string
  ) {
    setVertices(current =>
      current.filter(
        vertex =>
          vertex.id !== vertexId
      )
    );

    setEdges(current =>
      current.filter(
        edge =>
          edge.source !== vertexId
          && edge.target !== vertexId
      )
    );

    if (edgeSource === vertexId) {
      setEdgeSource('');
    }

    if (edgeTarget === vertexId) {
      setEdgeTarget('');
    }

    if (selectedNodeId === vertexId) {
      setSelectedNodeId(null);
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

  function resetGraph() {
    setVertices([]);
    setEdges([]);

    setEdgeSource('');
    setEdgeTarget('');

    setSelectedNodeId(null);

    setAnalysis(null);
    setError('');

    nextVertexId.current = 1;
    nextEdgeId.current = 1;
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

      setAnalysis(result);
    } catch (caughtError) {
      const message =
        caughtError instanceof Error
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
            Interaktivni test planarnosti
          </h1>

          <p className="intro">
            Kreirajte graf, pokrenite
            algoritam i pregledajte planar
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
                    event.key === 'Enter'
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
          </section>

          <section className="panel-section">
            <h2>
              Čvorovi
            </h2>

            {vertices.length === 0 ? (
              <p className="empty-text">
                Nema čvorova.
              </p>
            ) : (
              <ul className="item-list">
                {vertices.map(vertex => (
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
                ))}
              </ul>
            )}
          </section>

          <section className="panel-section">
            <h2>
              Grane
            </h2>

            {edges.length === 0 ? (
              <p className="empty-text">
                Nema grana.
              </p>
            ) : (
              <ul className="item-list">
                {edges.map(edge => {
                  const source =
                    vertices.find(
                      vertex =>
                        vertex.id
                        === edge.source
                    );

                  const target =
                    vertices.find(
                      vertex =>
                        vertex.id
                        === edge.target
                    );

                  return (
                    <li key={edge.id}>
                      <span>
                        {source?.label}
                        {' — '}
                        {target?.label}
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
                  );
                })}
              </ul>
            )}
          </section>
        </aside>

        <section className="canvas-panel">
          <GraphCanvas
            vertices={vertices}
            edges={edges}
            selectedNodeId={
              selectedNodeId
            }
            certificate={
              certificate
            }
            onSelectNode={
              setSelectedNodeId
            }
          />

          <div className="canvas-footer">
            <span>
              Čvorovi: {vertices.length}
            </span>

            <span>
              Grane: {edges.length}
            </span>

            {selectedNode && (
              <span>
                Odabrano: {
                  selectedNode.label
                }
              </span>
            )}
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

          {error && (
            <div className="error-card">
              {error}
            </div>
          )}

          {!analysis && !error && (
            <p className="empty-text">
              Rezultat će se prikazati
              nakon pokretanja algoritma.
            </p>
          )}

          {analysis?.planar && (
            <section className="result-card planar">
              <h2>
                Graf je planaran
              </h2>

              <p>
                Pronađen je validan
                rotation system.
              </p>

              <h3>
                Ciklični poredak grana
              </h3>

              <ul className="rotation-list">
                {Object.entries(
                  analysis.embedding
                    ?.clockwiseEdgesAroundVertex
                    ?? {}
                ).map(
                  ([vertexId, edgeIds]) => {
                    const vertex =
                      vertices.find(
                        candidate =>
                          candidate.id
                          === vertexId
                      );

                    return (
                      <li key={vertexId}>
                        <strong>
                          {
                            vertex?.label
                            ?? vertexId
                          }
                        </strong>

                        <span>
                          {
                            edgeIds.length
                              ? edgeIds.join(
                                  ', '
                                )
                              : '—'
                          }
                        </span>
                      </li>
                    );
                  }
                )}
              </ul>
            </section>
          )}

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

                <p className="muted-text">
                  Grane certifikata su
                  označene crvenom bojom.
                </p>

                <h3>
                  Branch čvorovi
                </h3>

                <ul>
                  {
                    analysis
                      .certificate
                      .branchVertexIds
                      .map(vertexId => {
                        const vertex =
                          vertices.find(
                            candidate =>
                              candidate.id
                              === vertexId
                          );

                        return (
                          <li key={vertexId}>
                            {
                              vertex?.label
                              ?? vertexId
                            }
                          </li>
                        );
                      })
                  }
                </ul>
              </section>
            )
          }
        </aside>
      </section>
    </main>
  );
}

export default App;