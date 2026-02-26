import React from "react";
import { Routes, Route, Link } from "react-router-dom";

import CreatePartyPage from "./pages/CreatePartyPage";
import GuestPage from "./pages/GuestPage";
import HostDashboardPage from "./pages/HostDashboardPage";

import "./App.css";

export default function App() {
  return (
    <div className="shell">
      <header className="topbar">
        <Link className="brand" to="/">
          Party UI
        </Link>
      </header>

      <main className="content">
        <Routes>
          <Route path="/" element={<CreatePartyPage />} />
          <Route path="/guest" element={<GuestPage />} />
          <Route path="/dashboard/:partyId" element={<HostDashboardPage />} />
        </Routes>
      </main>
    </div>
  );
}