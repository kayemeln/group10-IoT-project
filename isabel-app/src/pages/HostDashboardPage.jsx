import { useMemo, useState } from "react";
import { useParams, Link } from "react-router-dom";
import { addLog, getParty } from "../store/mockStore";

function fmt(iso) {
  try { return new Date(iso).toLocaleString(); } catch { return iso; }
}

export default function HostDashboardPage() {
  const { partyId } = useParams();
  const [tick, setTick] = useState(0);

  const party = useMemo(() => getParty(partyId), [partyId, tick]);

  if (!party) {
    return (
      <div className="card">
        <h1>Host Dashboard</h1>
        <div className="panel danger">Party not found.</div>
        <Link className="btn secondary" to="/">Back</Link>
      </div>
    );
  }

  const inviteUrl = `${window.location.origin}/guest?party=${party.id}&token=${party.inviteToken}`;

  function simulate(result) {
    addLog(party.id, {
      time: new Date().toISOString(),
      result,
      name: result === "ALLOW" ? "Matched Guest" : "Unknown",
      confidence: result === "ALLOW" ? 0.93 : 0.22
    });
    setTick((t) => t + 1);
  }

  return (
    <div className="card">
      <h1>Host Dashboard</h1>

      <div className="panel">
        <div style={{ fontSize: 18, fontWeight: 700 }}>{party.partyName}</div>
        <div className="muted">Host: {party.hostEmail}</div>

        <div style={{ marginTop: 10 }}>
          <div className="muted">Invite URL</div>
          <input readOnly value={inviteUrl} />
        </div>

        <div className="row" style={{ marginTop: 10 }}>
          <a className="btn secondary" href={inviteUrl} target="_blank" rel="noreferrer">
            Open Guest Link
          </a>
          <button className="btn" onClick={() => simulate("ALLOW")}>Simulate ALLOW</button>
          <button className="btn secondary" onClick={() => simulate("DENY")}>Simulate DENY</button>
        </div>
      </div>

      <div className="panel">
        <h2>Guests</h2>
        {party.guests.length === 0 ? (
          <div className="muted">No RSVPs yet.</div>
        ) : (
          <ul className="list">
            {party.guests.map((g) => (
              <li key={g.id} className="listItem">
                <div>
                  <b>{g.name}</b> <span className="muted">({g.numPhotos} photos)</span>
                  <div className="muted small">{fmt(g.time)}</div>
                  {g.photoNames?.length > 0 && (
                    <div className="muted small">Files: {g.photoNames.join(", ")}</div>
                  )}
                </div>
              </li>
            ))}
          </ul>
        )}
      </div>

      <div className="panel">
        <h2>Logs</h2>
        {party.logs.length === 0 ? (
          <div className="muted">No logs yet.</div>
        ) : (
          <div className="table">
            <div className="thead">
              <div>Time</div><div>Result</div><div>Name</div><div>Conf.</div>
            </div>
            {party.logs.map((l) => (
              <div key={l.id} className="trow">
                <div>{fmt(l.time)}</div>
                <div><span className={`pill ${l.result}`}>{l.result}</span></div>
                <div>{l.name || "-"}</div>
                <div>{typeof l.confidence === "number" ? l.confidence.toFixed(2) : "-"}</div>
              </div>
            ))}
          </div>
        )}
      </div>
    </div>
  );
}