import React, { useState } from "react";
import { useNavigate } from "react-router-dom";
import { generateClient } from "aws-amplify/api";
import { listParties } from "../graphql/queries"; 

export default function HomePage() {
  const client = generateClient();
  const [partyName, setPartyName] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");
  const navigate = useNavigate();

  const handleLogin = async (e) => {
    e.preventDefault();
    setError("");

    if (!partyName.trim() || !password.trim()) {
      setError("Enter party name and password");
      return;
    }

    try {
      const result = await client.graphql({
        query: listParties, 
        variables: {
          filter: {
            partyName: { eq: partyName.trim() },
          },
        },
      });

      const parties = result?.data?.listParties?.items || result?.data?.listPartys?.items || [];

      if (parties.length === 0) {
        setError("Party not found");
        return;
      }

      const party = parties.find((p) => p && p.password === password);

      if (!party) {
        setError("Incorrect password");
        return;
      }

      navigate(`/host-dashboard/${party.id}`);
    } catch (err) {
      console.error("LOGIN ERROR:", err);
      setError("Could not log in");
    }
  };

  return (
    <div style={{ padding: 24 }}>
      <h1>Party UI</h1>
      <p>Welcome. Choose an option below.</p>

      <div style={{ marginTop: 24, display: "grid", gap: 24, maxWidth: 500 }}>
        <div style={{ border: "1px solid #ddd", padding: 16, borderRadius: 8 }}>
          <h2>Log in to an existing party</h2>

          <form onSubmit={handleLogin}>
            <input
              type="text"
              placeholder="Enter party name"
              value={partyName}
              onChange={(e) => setPartyName(e.target.value)}
              style={{ width: "100%", marginBottom: 12 }}
            />

            <input
              type="password"
              placeholder="Enter password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              style={{ width: "100%", marginBottom: 12 }}
            />

            <button type="submit">Go to Host Dashboard</button>
          </form>

          {error && <p style={{ color: "crimson", marginTop: 12 }}>{error}</p>}
        </div>

        <div style={{ border: "1px solid #ddd", padding: 16, borderRadius: 8 }}>
          <h2>Create a new party</h2>
          <button type="button" onClick={() => navigate("/create-party")}>
            Create Party
          </button>
        </div>
      </div>
    </div>
  );
}