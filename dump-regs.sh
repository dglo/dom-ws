#!/bin/bash

se 50a < dump.se | sed 's/^DUMP //1' | tr ' ' '\n' | tr '=' '\t'

