import React from "react";
import { BrowserRouter, Routes, Route } from "react-router-dom";

import Layout from "./components/layout";
import HomePage from "./pages/HomePage";
import CreateParty from "./pages/CreateParty";
import HostDashboard from "./pages/HostDashboard";
import GuestUpload from "./pages/GuestUpload";

export default function App() {
  return (
    <BrowserRouter>
      <Routes>
        <Route element={<Layout />}>
          <Route path="/" element={<HomePage />} />
          <Route path="/create-party" element={<CreateParty />} />
          <Route path="/host-dashboard/:partyId" element={<HostDashboard />} />
          <Route path="/guest" element={<GuestUpload />} />
        </Route>
      </Routes>
    </BrowserRouter>
  );
}