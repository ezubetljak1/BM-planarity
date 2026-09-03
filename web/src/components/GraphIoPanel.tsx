import {
  useRef,
  useState,
  type ChangeEvent
} from 'react';

import {
  graphExamples,
  type GraphExample
} from '../examples';

import {
  GraphExampleModal
} from './GraphExampleModal';

import type {
  AnalysisSuccessResponse,
  GraphRequest
} from '../types';

interface GraphIoPanelProps {
  graph: GraphRequest;

  analysis:
    AnalysisSuccessResponse
    | null;

  onLoadGraph: (
    graph: GraphRequest
  ) => void;

  onError: (
    message: string
  ) => void;

  onExportPng: () => void;
  onExportSvg: () => void;
}

function cloneGraph(
  graph: GraphRequest
): GraphRequest {
  return {
    vertices:
      graph.vertices.map(vertex => ({
        ...vertex
      })),

    edges:
      graph.edges.map(edge => ({
        ...edge
      }))
  };
}

function isGraphRequest(
  value: unknown
): value is GraphRequest {
  if (
    typeof value !== 'object'
    || value === null
  ) {
    return false;
  }

  const candidate =
    value as Partial<GraphRequest>;

  if (
    !Array.isArray(candidate.vertices)
    || !Array.isArray(candidate.edges)
  ) {
    return false;
  }

  const verticesAreValid =
    candidate.vertices.every(vertex =>
      typeof vertex === 'object'
      && vertex !== null
      && typeof vertex.id === 'string'
      && typeof vertex.label === 'string'
    );

  const edgesAreValid =
    candidate.edges.every(edge =>
      typeof edge === 'object'
      && edge !== null
      && typeof edge.id === 'string'
      && typeof edge.source === 'string'
      && typeof edge.target === 'string'
    );

  return (
    verticesAreValid
    && edgesAreValid
  );
}

function downloadJson(
  filename: string,
  value: unknown
) {
  const blob =
    new Blob(
      [
        JSON.stringify(
          value,
          null,
          2
        )
      ],
      {
        type: 'application/json'
      }
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

export function GraphIoPanel({
  graph,
  analysis,
  onLoadGraph,
  onError,
  onExportPng,
  onExportSvg
}: GraphIoPanelProps) {
  const fileInputRef =
    useRef<HTMLInputElement | null>(
      null
    );

  const [
    examplesOpen,
    setExamplesOpen
  ] = useState(false);

  async function handleImport(
    event:
      ChangeEvent<HTMLInputElement>
  ) {
    const file =
      event.target.files?.[0];

    if (!file) {
      return;
    }

    try {
      const text =
        await file.text();

      const parsed =
        JSON.parse(text) as unknown;

      if (!isGraphRequest(parsed)) {
        throw new Error(
          'JSON fajl nema očekivanu strukturu.'
        );
      }

      onLoadGraph(
        cloneGraph(parsed)
      );
    } catch (caughtError) {
      const message =
        caughtError instanceof Error
          ? caughtError.message
          : 'Nije moguće učitati JSON fajl.';

      onError(message);
    } finally {
      event.target.value = '';
    }
  }

  function handleExampleSelect(
    example: GraphExample
  ) {
    onLoadGraph(
      cloneGraph(
        example.graph
      )
    );

    setExamplesOpen(false);
  }

  return (
    <section className="panel-section">
      <h2>
        Primjeri
      </h2>

      <button
        className="secondary-button full-width-button"
        type="button"
        onClick={() =>
          setExamplesOpen(true)
        }
      >
        Odaberi primjer grafa
      </button>

      <h3 className="panel-subheading">
        Uvoz i izvoz
      </h3>

      <div className="io-actions">
        <button
          className="secondary-button"
          type="button"
          onClick={() =>
            fileInputRef
              .current
              ?.click()
          }
        >
          Uvezi JSON
        </button>

        <details className="export-menu">
          <summary className="secondary-button export-menu-trigger">
            Izvezi
            <span aria-hidden="true">
              ▾
            </span>
          </summary>

          <div className="export-menu-content">
            <button
              type="button"
              onClick={() =>
                downloadJson(
                  'graph-input.json',
                  graph
                )
              }
            >
              Ulazni graf kao JSON
            </button>

            <button
              type="button"
              disabled={!analysis}
              onClick={() => {
                if (!analysis) {
                  return;
                }

                downloadJson(
                  'graph-analysis-result.json',
                  analysis
                );
              }}
            >
              Rezultat analize kao JSON
            </button>

            <button
              type="button"
              disabled={
                graph.vertices.length === 0
              }
              onClick={onExportPng}
            >
              Vizualizacija kao PNG
            </button>

            <button
              type="button"
              disabled={
                graph.vertices.length === 0
              }
              onClick={onExportSvg}
            >
              Vizualizacija kao SVG
            </button>
          </div>
        </details>
      </div>

      <input
        ref={fileInputRef}
        className="hidden-file-input"
        type="file"
        accept="application/json,.json"
        onChange={handleImport}
      />

      <GraphExampleModal
        open={examplesOpen}
        examples={graphExamples}
        onClose={() =>
          setExamplesOpen(false)
        }
        onSelect={handleExampleSelect}
      />
    </section>
  );
}
