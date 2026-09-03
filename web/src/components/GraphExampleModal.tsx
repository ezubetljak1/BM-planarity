import {
  useEffect
} from 'react';

import type {
  GraphExample,
  GraphExampleCategory
} from '../examples';

interface GraphExampleModalProps {
  open: boolean;
  examples: GraphExample[];

  onClose: () => void;
  onSelect: (
    example: GraphExample
  ) => void;
}

const categoryOrder:
  GraphExampleCategory[] = [
    'planar',
    'non-planar',
    'subdivision'
  ];

const categoryTitles:
  Record<
    GraphExampleCategory,
    string
  > = {
    planar: 'Planarni grafovi',
    'non-planar': 'Neplanarni grafovi',
    subdivision:
      'Kuratowskijeve subdivizije'
  };

export function GraphExampleModal({
  open,
  examples,
  onClose,
  onSelect
}: GraphExampleModalProps) {
  useEffect(
    () => {
      if (!open) {
        return undefined;
      }

      function handleKeyDown(
        event: KeyboardEvent
      ) {
        if (event.key === 'Escape') {
          onClose();
        }
      }

      window.addEventListener(
        'keydown',
        handleKeyDown
      );

      return () => {
        window.removeEventListener(
          'keydown',
          handleKeyDown
        );
      };
    },
    [open, onClose]
  );

  if (!open) {
    return null;
  }

  return (
    <div
      className="modal-backdrop"
      role="presentation"
      onMouseDown={event => {
        if (
          event.target
          === event.currentTarget
        ) {
          onClose();
        }
      }}
    >
      <section
        className="example-modal"
        role="dialog"
        aria-modal="true"
        aria-labelledby="example-modal-title"
      >
        <header className="modal-header">
          <div>
            <p className="modal-eyebrow">
              Demo grafovi
            </p>

            <h2 id="example-modal-title">
              Odaberite primjer
            </h2>

            <p className="modal-description">
              Učitajte gotov graf i odmah
              pokrenite Boyer–Myrvoldov
              test planarnosti.
            </p>
          </div>

          <button
            className="modal-close-button"
            type="button"
            aria-label="Zatvori"
            onClick={onClose}
          >
            ×
          </button>
        </header>

        <div className="example-groups">
          {
            categoryOrder.map(
              category => {
                const categoryExamples =
                  examples.filter(
                    example =>
                      example.category
                      === category
                  );

                return (
                  <section
                    className="example-group"
                    key={category}
                  >
                    <h3>
                      {
                        categoryTitles[
                          category
                        ]
                      }
                    </h3>

                    <div className="example-grid">
                      {
                        categoryExamples.map(
                          example => (
                            <button
                              className="example-card"
                              type="button"
                              key={example.id}
                              onClick={() =>
                                onSelect(
                                  example
                                )
                              }
                            >
                              <span className="example-card-header">
                                <strong>
                                  {example.name}
                                </strong>

                                <span
                                  className={
                                    example.expectedPlanar
                                      ? 'example-status planar'
                                      : 'example-status non-planar'
                                  }
                                >
                                  {
                                    example.expectedPlanar
                                      ? 'planaran'
                                      : 'neplanaran'
                                  }
                                </span>
                              </span>

                              <span className="example-description">
                                {
                                  example.description
                                }
                              </span>

                              <span className="example-meta">
                                {
                                  example.graph
                                    .vertices
                                    .length
                                } čvorova
                                {' · '}
                                {
                                  example.graph
                                    .edges
                                    .length
                                } grana
                              </span>
                            </button>
                          )
                        )
                      }
                    </div>
                  </section>
                );
              }
            )
          }
        </div>
      </section>
    </div>
  );
}
