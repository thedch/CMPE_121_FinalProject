# CMPE 121 Final Project, Spring 2017

This is the repository for the Rapsberry Pi side of the final project for CMPE 121 at UCSC. 

It contains code for both an 8 channel logic analyzer, and a 2 channel Oscilloscope, both using the openVG graphics library to display data.

It is designed to read signal data via UART, specifically from a Cypress microcontroller. The Cypress microcontroller has its own set of instructions to send data such that it will be properly read by the Pi -- these instructions are not in this repository. 
