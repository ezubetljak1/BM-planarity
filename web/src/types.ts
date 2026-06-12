export interface GraphVertex {
  id: string;
  label: string;
}

export interface GraphEdge {
  id: string;
  source: string;
  target: string;
}

export interface GraphRequest {
  vertices: GraphVertex[];
  edges: GraphEdge[];
}

export interface PlanarEmbedding {
  clockwiseEdgesAroundVertex: Record<string, string[]>;
}

export type KuratowskiType =
  | 'K5'
  | 'K3_3'
  | 'UNKNOWN';

export interface KuratowskiCertificate {
  type: KuratowskiType;
  edgeIds: string[];
  branchVertexIds: string[];
}

export interface AnalysisSuccessResponse {
  schemaVersion: number;
  ok: true;
  planar: boolean;

  vertices: GraphVertex[];
  edges: GraphEdge[];

  embedding?: PlanarEmbedding;
  certificate?: KuratowskiCertificate;
}

export interface AnalysisErrorResponse {
  schemaVersion: number;
  ok: false;

  error: {
    message: string;
  };
}

export type AnalysisResponse =
  | AnalysisSuccessResponse
  | AnalysisErrorResponse;