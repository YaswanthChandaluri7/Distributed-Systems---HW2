import grpc
import numpy as np
from concurrent import futures
import matrix_service_pb2
import matrix_service_pb2_grpc
import threading
from datetime import datetime

class DynamicThresholdMatrixServicer(matrix_service_pb2_grpc.MatrixServiceServicer):
    def __init__(self):
        self.matrix_rows = []
        self.matrix_cols = None
        self.client_has_completed = {}
        self.client_contributions = {}
        self.client_active_session = {}
        self.lock = threading.Lock()

    def SendRow(self, request, context):
        with self.lock:
            client_id = request.client_id
            values = list(request.values)
            timestamp = datetime.now().strftime("%H:%M:%S")
            client_name = 'A' if client_id == 1 else 'B'

            # Check if client has already COMPLETED a session
            if client_id in self.client_has_completed:
                error_msg = f"Client {client_name} has already completed data submission. Cannot send again."
                print(f" REJECTED: {error_msg}")
                return matrix_service_pb2.RowResponse(
                    success=False,
                    message=error_msg
                )

            print(f" Client {client_name} sending row with {len(values)} values: {values}")

            # Initialize matrix dimensions from first row
            if self.matrix_cols is None:
                self.matrix_cols = len(values)
                # print(f"[{timestamp}] Matrix columns detected: {self.matrix_cols}")
                print(f" Matrix columns detected: {self.matrix_cols}")

            # Validate row dimensions
            if len(values) != self.matrix_cols:
                error_msg = f"Row dimension mismatch. Expected {self.matrix_cols} columns, got {len(values)}"
                print(f" ERROR: {error_msg}")
                return matrix_service_pb2.RowResponse(
                    success=False,
                    message=error_msg
                )

            # Auto-assign row index
            row_index = len(self.matrix_rows)

            # Store the row
            self.matrix_rows.append(values)

            # Track client contribution
            if client_id not in self.client_contributions:
                self.client_contributions[client_id] = []
            self.client_contributions[client_id].append(row_index)

            # Mark client as having active session
            self.client_active_session[client_id] = True

            total_rows = len(self.matrix_rows)
            print(f" Row {row_index} stored successfully")
            print(f" Total rows received: {total_rows}")
            print(f" Client {client_name} rows so far: {self.client_contributions[client_id]}")

            return matrix_service_pb2.RowResponse(
                success=True,
                message=f"Row {row_index} received from client {client_name}",
                rows_received=total_rows,
                total_expected_rows=total_rows
            )

    def Query(self, request, context):
        with self.lock:
            client_id = request.client_id
            query_type = request.query_type
            timestamp = datetime.now().strftime("%H:%M:%S")
            client_name = 'A' if client_id == 1 else 'B'

            query_names = {1: "row_count", 2: "has_rank_at_least", 3: "has_determinant_at_least"}
            query_name = query_names.get(query_type, "unknown")
            print(f" Client {client_name} requesting {query_name}")

            # CHECK MATRIX DATA FIRST
            if not self.matrix_rows:
                print(f" No matrix data available for query")
                return matrix_service_pb2.QueryResponse(
                    success=False,
                    message="No matrix data available. Please send matrix data first."
                )

            # REMOVED: Premature completion marking
            # This was the problematic line that marked clients as completed after first query:
            # if client_id in self.client_active_session and client_id not in self.client_has_completed:
            #     self.client_has_completed[client_id] = True
            #     print(f"[{timestamp}] Client {client_name} marked as completed data submission")

            try:
                matrix = np.array(self.matrix_rows, dtype=float)
                print(f" Current matrix shape: {matrix.shape}")
                print(f" Current matrix:")
                for i, row in enumerate(matrix):
                    client_info = "A" if i in self.client_contributions.get(1, []) else "B"
                    print(f" Row {i} (Client {client_info}): {row}")

                if query_type == 1: 
                    result = matrix.shape[0]
                    print(f" Row count: {result}")
                    return matrix_service_pb2.QueryResponse(
                        success=True,
                        message=f"Matrix has {result} rows",
                        row_count=result
                    )

                elif query_type == 2: 
                    actual_rank = np.linalg.matrix_rank(matrix)
                    print(f" Matrix rank: {actual_rank}")
                    return matrix_service_pb2.QueryResponse(
                        success=True,
                        message=f"Actual rank: {actual_rank}",
                        rank=actual_rank
                    )

                elif query_type == 3:  
                    rows, cols = matrix.shape
                    if rows != cols:
                        error_msg = f"Dimensions not matched. Matrix is {rows}×{cols}, need square matrix for determinant."
                        print(f" {error_msg}")
                        return matrix_service_pb2.QueryResponse(
                            success=False,
                            message=error_msg
                        )

                    det = np.linalg.det(matrix)
                    print(f" Determinant: {det}")
                    return matrix_service_pb2.QueryResponse(
                        success=True,
                        message=f"Actual determinant: {det}",
                        determinant=det
                    )

                else:
                    return matrix_service_pb2.QueryResponse(
                        success=False,
                        message="Invalid query type. Use 1 (row count), 2 (rank), or 3 (determinant)"
                    )

            except Exception as e:
                error_msg = f"Error processing {query_name}: {str(e)}"
                print(f" ERROR: {error_msg}")
                return matrix_service_pb2.QueryResponse(
                    success=False,
                    message=error_msg
                )

def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    matrix_service_pb2_grpc.add_MatrixServiceServicer_to_server(
        DynamicThresholdMatrixServicer(), server
    )

    listen_addr = '[::]:50051'
    server.add_insecure_port(listen_addr)
    print(f"Listening on: {listen_addr}")

    server.start()
    try:
        server.wait_for_termination()
    except KeyboardInterrupt:
        server.stop(0)

if __name__ == '__main__':
    serve()
