import { initializeApp } from "https://www.gstatic.com/firebasejs/12.9.0/firebase-app.js";
import { getDatabase, ref, onValue, set }
  from "https://www.gstatic.com/firebasejs/12.9.0/firebase-database.js";

/* ===== FIREBASE CONFIG ===== */
const firebaseConfig = {
  apiKey: "AIzaSyCn1ocjTr9yE297jt8im1URPkOzzbJti4E",
  authDomain: "ash-smart-dustbin.firebaseapp.com",
  databaseURL: "https://ash-smart-dustbin-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "ash-smart-dustbin",
  storageBucket: "ash-smart-dustbin.firebasestorage.app",
  messagingSenderId: "447159296307",
  appId: "1:447159296307:web:20b063141c333bdeb0de46"
};

/* ===== INIT ===== */
const app = initializeApp(firebaseConfig);
const db = getDatabase(app);

/* ===== DOM ===== */
const fill = document.getElementById("fill");
const fullnessText = document.getElementById("fullness");
const statusText = document.getElementById("status");

const openBtn = document.getElementById("openBtn");
const closeBtn = document.getElementById("closeBtn");

/* ===== DB REFS ===== */
const fullnessRef = ref(db, "dustbin/fullness");
const servoRef = ref(db, "dustbin/servo");

/* ===== UI UPDATE ===== */
function updateUI(percent) {
  fullnessText.innerText = percent + "%";
  fill.style.transform = 'rotate(${percent * 1.8}deg)'; // ✅ FIXED

  if (percent >= 80) {
    fill.style.background = "#ff4444";
    statusText.innerText = "🚨 FULL";
  } else if (percent <= 20) {
    fill.style.background = "#33ff33";
    statusText.innerText = "✅ CLEANED";
  } else {
    fill.style.background = "#ffbb33";
    statusText.innerText = "🟢 OK";
  }
}

/* ===== REALTIME FULLNESS ===== */
onValue(fullnessRef, snapshot => {
  const value = snapshot.val() ?? 0;
  updateUI(value);
});

/* ===== BUTTONS ===== */
openBtn.onclick = () => set(servoRef, "OPEN");
closeBtn.onclick = () => set(servoRef, "CLOSE");
