// Importando e inicializando o Firebase
import { initializeApp } from "https://www.gstatic.com/firebasejs/9.6.11/firebase-app.js";
import { getDatabase, ref, set, onValue } from "https://www.gstatic.com/firebasejs/9.6.11/firebase-database.js";

// Sua configuração Firebase
const firebaseConfig = {
  apiKey: "AIzaSyCkIKGGIhbCrnO9dGJwO6vDZVmKXjKAJC0",
  authDomain: "pet-feeder-canil-default-rtdb.firebaseapp.com",
  databaseURL: "https://pet-feeder-canil-default-rtdb.firebaseio.com"
};

// Inicializando o Firebase
const app = initializeApp(firebaseConfig);
const database = getDatabase(app);

// Selecionando o botão e adicionando evento de clique
const activateMotorBtn = document.getElementById('activateMotor');

activateMotorBtn.addEventListener('click', () => {
    // Muda a cor do botão ao clicar (vermelho)
    activateMotorBtn.classList.add('activated');
    
    // Atualiza o status do motor no Firebase para 1 (ligar o motor)
    const motorStatusRef = ref(database, 'motorStatus');
    set(motorStatusRef, 1);
});

// Monitorando mudanças no nó `motorStatus`
const motorStatusRef = ref(database, 'motorStatus');

onValue(motorStatusRef, (snapshot) => {
    const motorStatus = snapshot.val();
    
    // Quando `motorStatus` é 1 (motor ativado)
    if (motorStatus === 1) {
        console.log("Motor ligado, aguardando desligamento...");
    }
    
    // Quando `motorStatus` volta para 0 (motor desligado), reseta a cor do botão
    if (motorStatus === 0) {
        activateMotorBtn.classList.remove('activated');
        console.log("Motor desligado, botão redefinido.");
    }
});

// Função para carregar os últimos 3 logs do Firebase
const logsRef = ref(database, 'logs');
const logsTable = document.getElementById('logsTable').getElementsByTagName('tbody')[0];

onValue(logsRef, (snapshot) => {
    const logs = snapshot.val();
    const logsArray = Object.entries(logs || {}).sort((a, b) => b[0] - a[0]).slice(0, 3);

    logsTable.innerHTML = ''; // Limpa a tabela
    logsArray.forEach(([key, value]) => {
        const row = logsTable.insertRow();
        const cell = row.insertCell(0);
        cell.textContent = value;
    });
});
