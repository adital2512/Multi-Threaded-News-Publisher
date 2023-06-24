# News Publisher

This project is a multi-threaded program for managing news articles produced by various producers and displayed by a screen manager. It utilizes bounded and unbounded buffers for inter-thread communication.

## Installation

To install and run this project, follow these steps:

1. Download the zip file of this project.
2. Extract it using 7zip or another app.
3. Change the config.txt file if you want.
4. Open linux termianl.
5. Navigate to the folder Multi-Threaded-News-Publisher.
6. type: make.
7. type: ./ex3.out config.txt 

## Description
The project consists of several components:

Producers: These are responsible for generating news articles. Each producer can produce multiple articles of different types (SPORTS, NEWS, or WEATHER). The number of articles to be produced by each producer is specified in the configuration file.

Dispatcher: The dispatcher coordinates the flow of articles from the producers to the screen manager. It reads the configuration file and creates the necessary data structures (bounded buffers) for inter-thread communication. The dispatcher receives articles from the producers and sends them to the appropriate unbounded buffers based on their type.

Unbounded Buffers: There are three unbounded buffers, one for each type of news article (SPORTS, NEWS, and WEATHER). The unbounded buffers store the articles received from the producers until they are consumed by the screen manager.

Screen Manager: The screen manager is responsible for displaying the news articles on the screen. It consumes articles from the unbounded buffers and displays them until all producers have finished producing articles and all articles have been displayed.
