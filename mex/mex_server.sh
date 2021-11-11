#!/bin/bash

echo "Start MEX Server..."
cd /home/jstec2/software/mex/mex
gunicorn --bind 0.0.0.0:8000 mex.wsgi
