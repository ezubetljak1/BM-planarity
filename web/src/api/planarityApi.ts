import type {
  AnalysisResponse,
  AnalysisSuccessResponse,
  GraphRequest
} from '../types';

export async function analyzeGraph(
  graph: GraphRequest
): Promise<AnalysisSuccessResponse> {
  const response = await fetch(
    '/api/planarity/analyze',
    {
      method: 'POST',

      headers: {
        'Content-Type': 'application/json'
      },

      body: JSON.stringify(graph)
    }
  );

  const body =
    await response.json() as AnalysisResponse;

  if (!response.ok || !body.ok) {
    const message =
      body.ok
        ? 'Planarity analysis failed.'
        : body.error.message;

    throw new Error(message);
  }

  return body;
}