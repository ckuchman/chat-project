#Programmer Name: Chris Kuchman
#Program Name: Chat Server
#Program Description: This program starts a server on the passed-in port and waits for a 
#client to connect and send a first message. The message is displayed to the user and the 
#user is prompted to respond. This continues back and forth till either the client or server 
#inputs \quit. This ends the connection and the server goes back to waiting for a new client.
#Course Name: CS 372-400
#Last Modified: 2/8/2020

import socketserver
import argparse

#This function starts up the server on the specified port and host waiting for a client to connect.
#This server will keep the server running indefinitely until a interrupt signal is sent from the server
#terminal.
#Ref: https://docs.python.org/3/library/socketserver.html
#Pre-conditions:
#The server port and host are available to the user running the function
#Post-conditions:
#A server will actively run and the process will not normally exit the function without a signal
#Return:
#None
def startServer(host, port):
    #Create the server, binding to localhost on specified port
    server = socketserver.TCPServer((host, port), MyTCPHandler)

    #Activate the server; this will keep running until you
    #interrupt the program with Ctrl-C
    server.serve_forever()


#This function sends a message from the provided server to the other end of the socket. The message always
#has the server handle put before the message. If the message input by the user is \quit then the message is
#still sent but the function returns a True boolean indicating that a quit is requested.
#Pre-conditions:
#This function is called by a server handler
#A socket connection with the input server and handler to client must exist
#Post-conditions:
#Message is sent to the other end of socket
#Return:
#A boolean indicating if the server is requesting a closure of the socket.
def sendMsg(server):
    handle = "Server3000"

    #Server user inputs message to send to client or /quit
    serverMsg = input("{}>".format(handle))

    #Adds handle
    serverMsg = handle + ">" + serverMsg 

    #Sends the message to the client
    server.request.sendall(serverMsg.encode('latin-1'))

    #If the server prompts to end the connection then a true boolean is returned
    if serverMsg.find("\\quit") != -1:
        return True
    else:
        return False


#This functions waits for a message to be received over the socket and then prints it to the server command line. If the message
#received is \quit then a boolean is returned by the function to indicate that the client is closing the connection.
#Pre-conditions:
#This function is called by a server handler
#A socket connection with the input server and handdler to client must exist
#The client must be sending messages in latin-1 format
#Post-conditions:
#None
#Return:
#A boolean indicating the client is reuesting a closure of the socket
def recvMsg(server):
    #The server always first gets a message from the client. The message comes in 'latin-1' format so the message is decoded accordingly.
    clientMsg = server.request.recv(1024).strip().decode('latin-1')

    #Displays the client message. As it already contains a \n the default for print() is removed
    print(clientMsg, end='')

    #If the client sends back a \quit message then the function returns a boolean indicating the quit request
    if clientMsg.find("\\quit") != -1:
        return True
    else:
        return False


#Basic handler class structure referenced from https://docs.python.org/3/library/socketserver.html
#This class is a child class of BaseRequestHandler, a python request superclass that defines the request handler interface
class MyTCPHandler(socketserver.BaseRequestHandler):

    #This overrides the default handle function which does nothing with the connection. This function is what is run when the 
    #socket is bound. The server waits for a message from the client, then prompts the user to end back their own. Messages must be sent back and forth
    #with neither side ever sending two messages ina  row. If either indicate a quit to the connection then the socket closes.
    #Pre-conditions:
    #A client must be connected to the socket
    #Post-conditions:
    #None
    #Return:
    #None
    def handle(self):
        #boolean to indicate a side wants to end the connection
        quit = False

        #loop that maintains chat loop until the command /quit is input or received from the client
        while not quit:

            #Message is recieved from client
            quit = recvMsg(self)
            
            #If the client didn't end the connection then the server prompts for a response
            if not quit:

                #Message is sent back from server
                quit = sendMsg(self)

        print("Client has disconnected")


#Standard TCP main program structure referenced from https://docs.python.org/3/library/socketserver.html
#Main program which startes up the server and watches port for any clients, creates a socket when they arrive.
#Pre-conditions:
#None
#Post-conditions:
#None
#Return:
#None
if __name__ == "__main__":

    #Parses the input to obtain the specified port number
    parser = argparse.ArgumentParser()
    parser.add_argument('port', type=int)
    args = parser.parse_args()

    print("Server started on port", args.port)

    #Starts port on specified host and port
    startServer("", args.port)
