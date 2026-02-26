import { useSearchParams } from "react-router-dom";
import { useMemo, useState } from "react";
import { addGuestRSVP, validateInvite } from "../store/mockStore";

export default function GuestPage() {
  const [params] = useSearchParams();
  const partyId = params.get("party") || "";
  const token = params.get("token") || "";

  const invite = useMemo(() => {
    if (!partyId || !token) return { ok: false, reason: "Missing party or token in URL" };
    return validateInvite(partyId, token);
  }, [partyId, token]);

  const [name, setName] = useState("");
  const [files, setFiles] = useState([]);
  const [done, setDone] = useState(false);
  const [err, setErr] = useState("");

  function submit(e) {
    e.preventDefault();
    setErr("");

    if (!invite.ok) return;
    if (!name.trim()) return setErr("Please enter your name.");
    if (files.length < 1) return setErr("Please select at least 1 photo.");

    addGuestRSVP(partyId, { name: name.trim(), photos: files });
    setDone(true);
  }

  if (!invite.ok) {
    return (
      <div className="card">
        <h1>Guest RSVP</h1>
        <div className="panel danger">
          <b>Invalid invite</b>
          <div className="muted">{invite.reason}</div>
        </div>
      </div>
    );
  }

  return (
    <div className="card">
      <h1>Guest RSVP</h1>
      <div className="panel">
        <div className="muted">You’re invited to:</div>
        <div style={{ fontSize: 18, fontWeight: 700 }}>{invite.party.partyName}</div>
      </div>

      {done ? (
        <div className="panel">
          RSVP submitted ✅ (UI-only, no upload yet)
        </div>
      ) : (
        <form onSubmit={submit} className="form">
          <input
            placeholder="Your name"
            value={name}
            onChange={(e) => setName(e.target.value)}
          />

          <input
            type="file"
            accept="image/*"
            multiple
            onChange={(e) => setFiles(Array.from(e.target.files || []))}
          />

          {err && <div className="error">{err}</div>}

          <button className="btn" type="submit">Submit RSVP</button>
        </form>
      )}
    </div>
  );
}