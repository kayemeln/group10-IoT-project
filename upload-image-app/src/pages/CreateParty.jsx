import React, { useState } from "react";
import { generateClient } from "aws-amplify/api";
import { createParty } from "../graphql/mutations";
import { Link } from "react-router-dom";

function generateJoinCode(length = 6) {
  const chars = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
  let result = "";
  for (let i = 0; i < length; i++) {
    result += chars[Math.floor(Math.random() * chars.length)];
  }
  return result;
}

export default function CreateParty() {
  const client = generateClient();
  const [partyName, setPartyName] = useState("");
  const [hostEmail, setHostEmail] = useState("");
  const [password, setPassword] = useState("");
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");
  const [createdParty, setCreatedParty] = useState(null);
  const [copied, setCopied] = useState(false);

  const handleSubmit = async (e) => {
    e.preventDefault();
    setError("");
    setLoading(true);

    try {
      const inviteToken = generateJoinCode();

      const result = await client.graphql({
        query: createParty,
        variables: {
          input: {
            partyName,
            hostEmail,
            inviteToken,
            password,
          },
        },
      });

      setCreatedParty(result.data.createParty);
      setPartyName("");
      setHostEmail("");
      setPassword("");
      setCopied(false);
    } catch (err) {
      console.error("CREATE PARTY ERROR:", err);
      setError("Failed to create party");
    } finally {
      setLoading(false);
    }
  };

  const inviteUrl =
    createdParty &&
    `${window.location.origin}/guest?party=${createdParty.id}&token=${createdParty.inviteToken}`;

  const handleCopy = async () => {
    try {
      if (!inviteUrl) return;
      await navigator.clipboard.writeText(inviteUrl);
      setCopied(true);
      setTimeout(() => setCopied(false), 2000);
    } catch (err) {
      console.error("Copy failed:", err);
    }
  };

  return (
    <div>
      <h2>Create Party</h2>

      <form onSubmit={handleSubmit}>
        <input
          type="text"
          placeholder="Party Name"
          value={partyName}
          onChange={(e) => setPartyName(e.target.value)}
          required
        />
        <input
          type="email"
          placeholder="Host Email"
          value={hostEmail}
          onChange={(e) => setHostEmail(e.target.value)}
          required
        />
        <input
          type="password"
          placeholder="Set a password"
          value={password}
          onChange={(e) => setPassword(e.target.value)}
          required
        />
        <button type="submit" disabled={loading}>
          {loading ? "Creating..." : "Create Party"}
        </button>
      </form>

      {error && <p>{error}</p>}

      {createdParty && (
        <div style={{ marginTop: 20 }}>
          <p>Party created.</p>

          <div style={{ display: "flex", gap: 10 }}>
            <input readOnly value={inviteUrl} style={{ width: "100%" }} />
            <button type="button" onClick={handleCopy}>
              {copied ? "Copied!" : "Copy"}
            </button>
          </div>

            <div style={{ marginTop: 12, display: "flex", gap: 10 }}>
            <button
              type="button"
              style={{ flex: 1 }}
              onClick={() => window.open(inviteUrl, "_blank")}
            >
              Open Guest Link
            </button>

            <Link to={`/host-dashboard/${createdParty.id}`} style={{ flex: 1 }}>
              <button type="button" style={{ width: "100%" }}>
                Host Dashboard
              </button>
            </Link>
          </div>
        </div>
      )}
    </div>
  );
}
