# Financial Data Exchange Center #



## Overview ##
The High-Performance Financial Data Center is a Linux-based real-time financial data exchange center that I developed in C++. It is designed to fetch and integrate stock data from multiple sources, such as Finnhub and Yahoo Finance, for over 200 companies. The data center uses MySQL and the Federated Engine for data management.


## Process Management and Scheduling ##
The project includes a module that automates process management and scheduling. This module transforms user-invoked processes into daemon processes, shielding them from irrelevant signal interference and ensuring stable operation. It also includes a process check module that automatically scans each business process, promptly handles exceptions, and records exception information for later analysis.

## Transmission Module ##
The project includes a TCP transmission framework that ensures the security and integrity of file transfers from the external network to the server. This system supports both incremental and batch transmission modes to improve operational efficiency. It also includes a file management module that regularly cleans, compresses, and transfers expired files to save memory space and maintain efficient server operation.

## ternal Network File Transfer Server and Client ##
The project includes a TCP transmission framework and a new encapsulation protocol to solve the TCP transmission stick package problem for the internal network environment. It also includes a high-performance concurrent server that uses IO port multiplexing technology to serve multiple clients' file upload and download needs. The server regularly checks client connections to ensure they are normal and that server resources are not wasted. The client supports uploading/downloading files to/from the server and supports both batch and incremental upload modes. It also includes a timing module that can start the client to complete tasks at a specified time.

## Database Management System ##
The project includes a SQL operating system based on libmysql for quick database deployment. This system supports the automatic organization and storage of various file formats (such as XML, CSV, JSON) into the MySQL database. It also supports the automatic download of database files in a fixed format and supports both batch and incremental modes. The project also includes a distributed database developed using the Federated Engine, which allows sub-databases to update from the main database based on keywords. The incremental update module I developed for sub-database synchronization saves 20% of storage space and improves update speed by 40% compared to full updates.


## End ##
The High-Performance Financial Data Center project is a comprehensive solution that addresses various needs in the financial data exchange field. It ensures efficient data management, secure and efficient data transmission, and robust process management.
