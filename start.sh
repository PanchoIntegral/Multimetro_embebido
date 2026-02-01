#!/bin/bash

# Script de inicio rápido para el sistema de multímetro
# Este script inicia tanto el backend como el frontend

echo "🚀 Iniciando Sistema de Multímetro..."
echo ""

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Verificar que estamos en el directorio correcto
if [ ! -d "backend" ] || [ ! -d "frontend" ]; then
    echo -e "${RED}❌ Error: Este script debe ejecutarse desde el directorio raíz del proyecto${NC}"
    exit 1
fi

# Función para verificar si un puerto está en uso
check_port() {
    lsof -i :$1 > /dev/null 2>&1
    return $?
}

# Verificar puertos
if check_port 5000; then
    echo -e "${YELLOW}⚠️  Puerto 5000 ya está en uso. Por favor cierra la aplicación que lo está usando.${NC}"
    exit 1
fi

if check_port 5173; then
    echo -e "${YELLOW}⚠️  Puerto 5173 ya está en uso. Por favor cierra la aplicación que lo está usando.${NC}"
    exit 1
fi

# Iniciar backend
echo -e "${GREEN}📡 Iniciando Backend...${NC}"
cd backend

# Verificar si existe el entorno virtual
if [ ! -d "venv" ]; then
    echo -e "${YELLOW}⚠️  Entorno virtual no encontrado. Creando...${NC}"
    python3 -m venv venv
    source venv/bin/activate
    echo -e "${GREEN}📦 Instalando dependencias...${NC}"
    pip install -r requirements.txt
else
    source venv/bin/activate
fi

# Iniciar backend en segundo plano
python app.py > ../backend.log 2>&1 &
BACKEND_PID=$!
echo -e "${GREEN}✅ Backend iniciado (PID: $BACKEND_PID)${NC}"

cd ..

# Esperar un momento para que el backend inicie
sleep 2

# Iniciar frontend
echo -e "${GREEN}🎨 Iniciando Frontend...${NC}"
cd frontend

# Verificar si node_modules existe
if [ ! -d "node_modules" ]; then
    echo -e "${YELLOW}⚠️  Dependencias no instaladas. Instalando...${NC}"
    npm install
fi

# Iniciar frontend en segundo plano
npm run dev > ../frontend.log 2>&1 &
FRONTEND_PID=$!
echo -e "${GREEN}✅ Frontend iniciado (PID: $FRONTEND_PID)${NC}"

cd ..

# Guardar PIDs para poder detener después
echo $BACKEND_PID > .backend.pid
echo $FRONTEND_PID > .frontend.pid

echo ""
echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}✨ Sistema de Multímetro Iniciado Exitosamente ✨${NC}"
echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "📡 Backend:  ${GREEN}http://localhost:5000${NC}"
echo -e "🎨 Frontend: ${GREEN}http://localhost:5173${NC}"
echo ""
echo -e "📋 Logs:"
echo -e "   Backend:  tail -f backend.log"
echo -e "   Frontend: tail -f frontend.log"
echo ""
echo -e "🛑 Para detener el sistema: ${YELLOW}./stop.sh${NC}"
echo ""
echo -e "${YELLOW}⚠️  IMPORTANTE: Verifica que el puerto serial en backend/config.py sea correcto${NC}"
echo ""

# Abrir navegador automáticamente (opcional)
sleep 3
if command -v open &> /dev/null; then
    open http://localhost:5173
elif command -v xdg-open &> /dev/null; then
    xdg-open http://localhost:5173
fi
