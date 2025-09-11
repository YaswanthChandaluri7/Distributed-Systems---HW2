import grpc
import matrix_service_pb2
import matrix_service_pb2_grpc

class DynamicThresholdClient:
    def __init__(self, client_id, server_address='localhost:50051'):
        self.client_id = client_id
        self.channel = grpc.insecure_channel(server_address)
        self.stub = matrix_service_pb2_grpc.MatrixServiceStub(self.channel)
        self.planned_rows = 0
        self.rows_sent = 0
        self.has_sent_all = False
        self.planned_rows_set = False

    def enter_number_of_rows(self):
        """Option 1: Enter number of rows willing to send to server"""
        if self.planned_rows_set:
            print("You have already decided how many rows you are willing to send.")
            return
        if self.has_sent_all:
            print("You already sent all planned rows.")
            return
        try:
            num_rows = int(input("Enter number of rows willing to send: "))
            if num_rows <= 0:
                print("Number of rows must be positive.")
                return
            self.planned_rows = num_rows
            self.planned_rows_set = True
            print(f"Number of rows to send set to {num_rows}.")
        except ValueError:
            print("Invalid input. Please enter a positive integer.")

    def send_row_to_server(self):
        """Option 2: Send rows one by one to server"""
        if not self.planned_rows_set:
            print("Please enter number of rows first (option 1) before sending rows.")
            return
        if self.has_sent_all:
            print("You have already sent all planned rows.")
            return
        if self.rows_sent >= self.planned_rows:
            print("You have already sent all planned rows.")
            self.has_sent_all = True
            return

        try:
            row_input = input(f"Enter values of row {self.rows_sent + 1} (space-separated): ")
            values = [float(x) for x in row_input.strip().split()]
            if not values:
                print("Please enter at least one value.")
                return
        except ValueError:
            print("Invalid input. Please enter numbers separated by spaces.")
            return

        try:
            request = matrix_service_pb2.MatrixRow(
                client_id=self.client_id,
                row_index=0,  # Server auto-assigns
                values=values
            )
            response = self.stub.SendRow(request)
            if response.success:
                self.rows_sent += 1
                print(f"Row {self.rows_sent} sent successfully.")
                if self.rows_sent == self.planned_rows:
                    print("All planned rows sent.")
                    self.has_sent_all = True
            else:
                print(f"Send row failed: {response.message}")
                if "already completed" in response.message:
                    self.has_sent_all = True
        except grpc.RpcError as e:
            print(f"RPC Error sending row: {e}")

    def query_rank(self):
        """Option 3: Query if matrix has rank at least r"""
        try:
            r = int(input("Enter rank  r: "))
        except ValueError:
            print("Invalid input. Please enter an integer.")
            return

        try:
            request = matrix_service_pb2.QueryRequest(
                client_id=self.client_id,
                query_type=2
            )
            response = self.stub.Query(request)
            if response.success:
                actual_rank = response.rank
                result = actual_rank >= r
                # print(f"Has_AtLeast_rank({r}): {result}")
                print(f"{result}")
            else:
                print(f"Query failed: {response.message}")
        except grpc.RpcError as e:
            print(f"RPC Error in rank query: {e}")

    def query_determinant(self):
        """Option 4: Query if matrix has determinant at least d"""
        try:
            d = float(input("Enter determinant  value: "))
        except ValueError:
            print("Invalid input. Please enter a number.")
            return

        try:
            request = matrix_service_pb2.QueryRequest(
                client_id=self.client_id,
                query_type=3
            )
            response = self.stub.Query(request)
            if response.success:
                actual_det = response.determinant
                result = (actual_det) >= d
                # print(f"Has_ Atleast_Determinant({d}): {result}")
                print(f"{result}")
            else:
                print(f"Query failed: {response.message}")
        except grpc.RpcError as e:
            print(f"RPC Error in determinant query: {e}")

    def exit_program(self):
        """Option 5: Exit the client program"""
        # print("Exiting client...")
        self.channel.close()

    def run_interactive_session(self):
        """Run interactive client session"""
        client_name = 'A' if self.client_id == 1 else 'B'
        print(f"Starting Client {client_name} (ID: {self.client_id})")

        # print(f"\n{'='*40}")
        print(f"\n")
        print(f"CLIENT {client_name} - MATRIX OPERATIONS")
        # print(f"\n")
        # print(f"{'='*40}")
        print("1. Enter number of rows to send")
        print("2. Send a single row to server")
        print("3. Has rank at least r")
        print("4. Has determinant at least d")
        print("5. Exit")
        
        while True:
            
            # print(f"{'='*40}")
            
            # Show current status

            print(f"\n")

            # if self.planned_rows_set and not self.has_sent_all:
            #     print(f"Status: {self.rows_sent}/{self.planned_rows} rows sent")
            # elif self.has_sent_all:
            #     print("Status: All planned rows sent")
            
            try:
                choice = input("Enter your choice (1-5): ").strip()

                if choice == '1':
                    print(f"\n CLIENT {client_name} - SET ROW COUNT")
                    print("-" * 30)
                    self.enter_number_of_rows()
                    
                elif choice == '2':
                    print(f"\n CLIENT {client_name} - SEND ROW")
                    print("-" * 30)
                    self.send_row_to_server()
                    
                elif choice == '3':
                    print(f"\n CLIENT {client_name} - RANK QUERY")
                    print("-" * 30)
                    self.query_rank()
                    
                elif choice == '4':
                    print(f"\n CLIENT {client_name} - DETERMINANT QUERY")
                    print("-" * 30)
                    self.query_determinant()
                    
                elif choice == '5':
                    self.exit_program()
                    break
                    
                else:
                    print("Invalid choice. Please enter 1, 2, 3, 4, or 5.")
                    
            except KeyboardInterrupt:
                # print(f"\nClient {client_name} interrupted. Exiting...")
                break

def main():
    import sys
    if len(sys.argv) != 2:
        print("Usage: python client.py <client_id>")
        print("Example: python client.py 1")
        sys.exit(1)
        
    try:
        client_id = int(sys.argv[1])
        if client_id not in [1, 2]:
            print("Client ID must be 1 or 2")
            sys.exit(1)
        client = DynamicThresholdClient(client_id)
        client.run_interactive_session()
    except ValueError:
        print("Client ID must be a number (1 or 2)")

if __name__ == '__main__':
    main()
