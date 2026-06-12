import {
  useState
} from 'react';

import type {
  GraphEdge,
  GraphRequest,
  GraphVertex
} from '../types';

interface GraphTextInputPanelProps {
  onLoadGraph: (
    graph: GraphRequest
  ) => void;

  onError: (
    message: string
  ) => void;
}

const EXAMPLE_TEXT = `x1,x2,x3

x1-x2
x2-x3`;

function canonicalEdgeKey(
  firstVertexId: string,
  secondVertexId: string
) {
  return firstVertexId
    < secondVertexId
      ? `${firstVertexId}|${secondVertexId}`
      : `${secondVertexId}|${firstVertexId}`;
}

export function parseGraphText(
  input: string
): GraphRequest {
  const lines =
    input
      .split(/\r?\n/)
      .map(line =>
        line.trim()
      )
      .filter(line =>
        line.length > 0
      );

  if (lines.length === 0) {
    throw new Error(
      'Unesite listu čvorova.'
    );
  }

  const labels =
    lines[0]
      .split(',')
      .map(label =>
        label.trim()
      );

  if (
    labels.some(
      label =>
        label.length === 0
    )
  ) {
    throw new Error(
      'Oznake čvorova ne smiju biti prazne.'
    );
  }

  const duplicateLabel =
    labels.find(
      (
        label,
        index
      ) =>
        labels.indexOf(label)
        !== index
    );

  if (duplicateLabel) {
    throw new Error(
      `Čvor sa oznakom "${duplicateLabel}" naveden je više puta.`
    );
  }

  const invalidLabel =
    labels.find(label =>
      label.includes('-')
    );

  if (invalidLabel) {
    throw new Error(
      `Oznaka čvora "${invalidLabel}" ne smije sadržavati znak "-".`
    );
  }

  const vertices:
    GraphVertex[] =
      labels.map(
        (
          label,
          index
        ) => ({
          id:
            `v${index + 1}`,

          label
        })
      );

  const vertexIdByLabel =
    new Map<
      string,
      string
    >();

  vertices.forEach(vertex => {
    vertexIdByLabel.set(
      vertex.label,
      vertex.id
    );
  });

  const edges:
    GraphEdge[] = [];

  const existingEdgeKeys =
    new Set<string>();

  lines
    .slice(1)
    .forEach(
      (
        line,
        index
      ) => {
        const lineNumber =
          index + 2;

        const parts =
          line
            .split('-')
            .map(part =>
              part.trim()
            );

        if (
          parts.length !== 2
          || parts.some(
            part =>
              part.length === 0
          )
        ) {
          throw new Error(
            `Neispravna grana u liniji ${lineNumber}: "${line}". Očekivan format je x1-x2.`
          );
        }

        const [
          sourceLabel,
          targetLabel
        ] = parts;

        const sourceId =
          vertexIdByLabel.get(
            sourceLabel
          );

        const targetId =
          vertexIdByLabel.get(
            targetLabel
          );

        if (!sourceId) {
          throw new Error(
            `Nepoznat čvor "${sourceLabel}" u liniji ${lineNumber}.`
          );
        }

        if (!targetId) {
          throw new Error(
            `Nepoznat čvor "${targetLabel}" u liniji ${lineNumber}.`
          );
        }

        if (sourceId === targetId) {
          throw new Error(
            `Self-loop nije podržan u liniji ${lineNumber}: "${line}".`
          );
        }

        const edgeKey =
          canonicalEdgeKey(
            sourceId,
            targetId
          );

        if (
          existingEdgeKeys.has(
            edgeKey
          )
        ) {
          throw new Error(
            `Paralelna grana je navedena u liniji ${lineNumber}: "${line}".`
          );
        }

        existingEdgeKeys.add(
          edgeKey
        );

        edges.push({
          id:
            `e${edges.length + 1}`,

          source:
            sourceId,

          target:
            targetId
        });
      }
    );

  return {
    vertices,
    edges
  };
}

export function GraphTextInputPanel({
  onLoadGraph,
  onError
}: GraphTextInputPanelProps) {
  const [
    text,
    setText
  ] = useState('');

  function handleLoad() {
    try {
      onLoadGraph(
        parseGraphText(text)
      );
    } catch (caughtError) {
      const message =
        caughtError
        instanceof Error
          ? caughtError.message
          : 'Nije moguće parsirati tekstualni unos.';

      onError(message);
    }
  }

  return (
    <section className="panel-section">
      <h2>
        Tekstualni unos
      </h2>

      <p className="text-input-help">
        Prva linija sadrži čvorove
        odvojene zarezima. Svaka naredna
        linija predstavlja jednu granu.
      </p>

      <textarea
        className="graph-textarea"
        rows={9}
        value={text}
        placeholder={`x1,x2,x3

x1-x2
x2-x3`}
        onChange={event =>
          setText(
            event.target.value
          )
        }
      />

      <div className="text-input-actions">
        <button
          type="button"
          onClick={handleLoad}
        >
          Učitaj tekstualni graf
        </button>

        <button
          className="secondary-button"
          type="button"
          onClick={() =>
            setText(
              EXAMPLE_TEXT
            )
          }
        >
          Popuni primjer
        </button>
      </div>
    </section>
  );
}