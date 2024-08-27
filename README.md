<div align="center">
  <img src="https://i.giphy.com/media/v1.Y2lkPTc5MGI3NjExd3ozaG9wczd2MmdsNHk0czgyc2pqcGJkc3Q3d2Nwb2Q2ZTVuNWg4diZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/116S6GJEkPw8YE/giphy.gif" alt="File Sharing GIF" width="100%" height="250">
  <h1>FTP</h1>
  <p>A simple yet robust FTP client-server system supporting file transfers, user management, and secure access.</p>

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
![GitHub contributors](https://img.shields.io/github/contributors/Yassineelg/FTP)
</div>

## Table of Contents

- [🌱 Introduction](#-introduction)
- [🏞️ Project Structure](#-project-structure)
- [🔧 Installation](#-installation)
- [🌍 Usage](#-usage)
- [🌟 Features](#-features)
- [📦 Dependencies](#-dependencies)
- [⚙️ Configuration](#-configuration)
- [📚 Documentation](#-documentation)
- [🌱 Examples](#-examples)
- [🔍 Troubleshooting](#-troubleshooting)
- [👨‍💻 Contributors](#-contributors)
- [📝 License](#-license)

## 🌱 Introduction

FTP is a project focused on developing a client-server FTP system. This system allows for file transfers, user authentication, and management of user directories in a secure and efficient manner. The project is structured to be easy to use and extend, with the potential for adding features such as multi-threading, enhanced security, and more.

## 🏞️ Project Structure

The project is organized as follows:

```plaintext
.
├── README.md
├── src
│   ├── client
│   └── server
├── infra
│   ├── Dockerfile
│   └── Taskfile.yml
└── docs
```

- `src`: Contains the source code for the FTP client and server.
- `infra`: Contains infrastructure-related files, including Docker and Taskfile configurations.
- `docs`: Includes all project-related documentation.

## 🔧 Installation

To set up the project locally, follow these steps:

1. **Clone the repository**:
   ```sh
   git clone https://github.com/yassineelg/ftp.git
   cd ftp
   ```

2. **Install Taskfile**:
   If Taskfile is not installed on your device, download it from [here](https://taskfile.dev/#/installation) and follow the installation instructions.

3. **Install Docker**:
   If Docker is not installed on your device, download it from [here](https://www.docker.com/products/docker-desktop) and follow the installation instructions.

4. **Run the Taskfile**:
   Taskfile will handle the Docker setup and running the containers. Run the following command:
   ```sh
   task up
   ```

## 🌍 Usage

Once the containers are up and running, the FTP services can be accessed:

- **Client**: Use the provided client executable to interact with the server.
- **Server**: The server will be running inside the Docker container, handling client requests.

Example client commands:

- **Ping-Pong Test**:
  ```sh
  ./lpf ip:port -ping
  ```

- **Upload a File**:
  ```sh
  ./lpf ip:port -upload filename
  ```

- **Download a File**:
  ```sh
  ./lpf ip:port -download filename
  ```

## 🌟 Features

- **Multi-user Support**: Handle multiple users with personalized directories.
- **Secure Authentication**: User authentication with password storage.
- **Folder Management**: Create, list, delete, and rename directories.
- **Comprehensive Logging**: Log all user commands, IP addresses, and potential errors.

## 📦 Dependencies

- **Docker**: Used to containerize the application, making it easier to deploy and manage.
- **Taskfile**: Manages tasks such as setting up the environment, running Docker containers, and more.

## ⚙️ Configuration

- **User Authentication File**: Located at `very_safe_trust_me_bro.txt` inside the Docker container.
  - Format: `username:password`.
- **Logging**: Logs are saved within the Docker container, containing details of each command executed.

## 📚 Documentation

Refer to the `docs` directory for detailed documentation on setup, usage, and feature descriptions.

## 🌱 Examples

Example usage scenarios and code snippets will be provided as the project develops.

## 🔍 Troubleshooting

- **Common Issues**:
  - Ensure the Docker container is running before executing client commands.
  - Verify network configurations (IP, port) are correctly set.

## 👨‍💻 Contributors

- **Elarif INZOUDINE** [![GitHub](https://img.shields.io/badge/-GitHub-181717?style=flat-square&logo=github&logoColor=white&link=https://github.com/harrysCTB)](https://github.com/harrysCTB) [![LinkedIn](https://img.shields.io/badge/-LinkedIn-0077B5?style=flat-square&logo=linkedin&logoColor=white&link=https://www.linkedin.com/in/elarif-inzoudine)](https://www.linkedin.com/in/elarif-inzoudine)
- **Jordan SISSILIAN** [![GitHub](https://img.shields.io/badge/-GitHub-181717?style=flat-square&logo=github&logoColor=white&link=https://github.com/jordan-sissilian)](https://github.com/jordan-sissilian) [![LinkedIn](https://img.shields.io/badge/-LinkedIn-0077B5?style=flat-square&logo=linkedin&logoColor=white&link=https://www.linkedin.com/in/sissilian)](https://www.linkedin.com/in/sissilian/)
 - **Yassine EL GHERRABI** [![GitHub](https://img.shields.io/badge/-GitHub-181717?style=flat-square&logo=github&logoColor=white&link=https://github.com/yassineelg)](https://github.com/yassineelg) [![LinkedIn](https://img.shields.io/badge/-LinkedIn-0077B5?style=flat-square&logo=linkedin&logoColor=white&link=https://www.linkedin.com/in/yassine-el-gherrabi)](https://www.linkedin.com/in/yassine-el-gherrabi)

💡 **Feel free to contribute to this project.** Here's how you can do it:

- Fork the project

- Create a pull request (PR)

- Ensure you use conventional commits

## 📝 License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.