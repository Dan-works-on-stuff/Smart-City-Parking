# General vision:
The application uses a central server and a multitude of mini-servers called "FloorMasters" to monitor parking spaces in a multi-story car park OR parking lots throughout an entire city. Each FloorMaster will manage a specific floor/area, detecting in real time the occupied and free spaces, using sensors mounted above each parking space (sensors) that will respond to the FloorMaster, then reporting the information to the server. The server centralizes the data and provides users with an interface to view the availability of spaces. In addition to the FloorMaster and sensors, the application also includes an Assigner. This communicates with the server and the FloorMasters to manage resource allocation. The application can now manage up to 10 floors/car parks, with 100 spaces each, totaling 1000 parking spaces.

# Application Objectives

1. Real-time Monitoring: Rapid identification and reporting of the status of each parking space using proximity sensors mounted above each parking space.

2. Centralization: Aggregation of information about all floors on a central server.

3. Scalability: The ability to add new floors or parking spaces up to the limit defined by MAX_PARCARI, without affecting system performance.

4. Presentation of information in a simple web interface.

# Applied Technologies:

Communication is carried out through TCP/IP technology to ensure an individual connection with each process. The UDP communication method is not necessary because the server has nothing to communicate with all connected processes at the same time, or with other unassociated instances.
The FloorMasters, which have the entire floor/parking lot under their control, will receive parking space status changes from each sensor, and then use an epoll socket to communicate the changes to the central server. The established communication protocol is IPv4 because the advanced functionality of the IPv6 protocol is not necessary in this situation, the program being one that focuses on simplicity and intelligibility. Also, the encryption that IPv6 offers is not required. This protocol could still be implemented in the future given the modularity of the code. Clients communicate with the server (and with each other) through the EPOLL function because it is necessary to keep the program efficient and fast when a large number of clients communicate concurrently.
The Front-End part, the web application is made with HTML, CSS and JavaScript.

# Application Structure:

![diagrama proiect](https://github.com/user-attachments/assets/81086b36-3402-4c65-8482-0f2dfffc9e6b)
