import {
  useRef,
  type ChangeEvent
} from 'react';

import {
  k33Example,
  triangleExample
} from '../examples';

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
  onError
}: GraphIoPanelProps) {
  const fileInputRef =
    useRef<HTMLInputElement | null>(
      null
    );

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

  return (
    <section className="panel-section">
      <h2>
        Primjeri i JSON
      </h2>

      <div className="button-grid">
        <button
          className="secondary-button"
          type="button"
          onClick={() =>
            onLoadGraph(
              cloneGraph(
                triangleExample
              )
            )
          }
        >
          Učitaj trougao
        </button>

        <button
          className="secondary-button"
          type="button"
          onClick={() =>
            onLoadGraph(
              cloneGraph(
                k33Example
              )
            )
          }
        >
          Učitaj K3,3
        </button>

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

        <button
          className="secondary-button"
          type="button"
          onClick={() =>
            downloadJson(
              'graph-input.json',
              graph
            )
          }
        >
          Izvezi ulazni JSON
        </button>

        <button
          className="secondary-button"
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
          Izvezi rezultat
        </button>
      </div>

      <input
        ref={fileInputRef}
        className="hidden-file-input"
        type="file"
        accept="application/json,.json"
        onChange={handleImport}
      />
    </section>
  );
}