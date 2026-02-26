import { useState } from "react";
import { Link } from "react-router-dom";
import { createParty } from "../store/mockStore";

export default function CreatePartyPage() {
  const [partyName, setPartyName] = useState("");
  const [hostEmail, setHostEmail] = useState("");
  const [created, setCreated] = useState(null);

  function handleCreate(e) {
    e.preventDefault();
    if (!partyName.trim() || !hostEmail.trim()) return;

    const party = createParty({
      partyName: partyName.trim(),
      hostEmail: hostEmail.trim(),
    });

    setCreated(party);
    setPartyName("");
    setHostEmail("");
  }

  const inviteUrl =
    created &&
    `${window.location.origin}/guest?party=${created.id}&token=${created.inviteToken}`;

  return (
    <div className="card">
      <h1>Create Party (Host)</h1>

      <form onSubmit={handleCreate} className="form">
        <input
          placeholder="Party name"
          value={partyName}
          onChange={(e) => setPartyName(e.target.value)}
        />
        <input
          placeholder="Host email"
          value={hostEmail}
          onChange={(e) => setHostEmail(e.target.value)}
        />
        <button className="btn" type="submit">Create</button>
      </form>

      {created && (
        <div className="panel">
          <div><b>Party created </b></div>

          <div style={{ marginTop: 10 }}>
            <div className="muted">Invite URL</div>
            <input readOnly value={inviteUrl} />
          </div>

          <div className="row" style={{ marginTop: 10 }}>
            <a className="btn secondary" href={inviteUrl} target="_blank" rel="noreferrer">
              Open Guest Link
            </a>
            <Link className="btn" to={`/dashboard/${created.id}`}>
              Host Dashboard
            </Link>
          </div>
        </div>
      )}
    </div>
  );
}