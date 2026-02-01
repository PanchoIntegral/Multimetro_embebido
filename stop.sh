#!/bin/bash

# Script para detener el sistema de multímetro

echo "🛑 Deteniendo Sistema de Multímetro..."

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

# Leer PIDs
if [ -f ".backend.pid" ]; then
    BACKEND_PID=$(cat .backend.pid)
    if ps -p $BACKEND_PID > /dev/null 2>&1; then
        kill $BACKEND_PID
        echo -e "${GREEN}✅ Backend detenido (PID: $BACKEND_PID)${NC}"
    fi
    rm .backend.pid
fi

if [ -f ".frontend.pid" ]; then
    FRONTEND_PID=$(cat .frontend.pid)
    if ps -p $FRONTEND_PID > /dev/null 2>&1; then
        kill $FRONTEND_PID
        echo -e "${GREEN}✅ Frontend detenido (PID: $FRONTEND_PID)${NC}"
    fi
    rm .frontend.pid
fi

# Limpiar procesos huérfanos en los puertos
lsof -ti:5000 | xargs kill -9 2>/dev/null
lsof -ti:5173 | xargs kill -9 2>/dev/null

echo -e "${GREEN}✨ Sistema detenido completamente${NC}"
