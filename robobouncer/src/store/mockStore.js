import { v4 as uuidv4 } from "uuid";

const LS_KEY = "party_ui_mock_store_v1";

function load() {
  try {
    const raw = localStorage.getItem(LS_KEY);
    if (!raw) return { parties: {} };
    return JSON.parse(raw);
  } catch {
    return { parties: {} };
  }
}

function save(state) {
  localStorage.setItem(LS_KEY, JSON.stringify(state));
}

let state = load();

export function createParty({ partyName, hostEmail }) {
  const partyId = uuidv4();
  const inviteToken = uuidv4().slice(0, 12);

  state.parties[partyId] = {
    id: partyId,
    partyName,
    hostEmail,
    inviteToken,
    guests: [],
    logs: [],
  };

  save(state);
  return state.parties[partyId];
}

export function getParty(partyId) {
  return state.parties[partyId] ?? null;
}

export function validateInvite(partyId, token) {
  const p = getParty(partyId);
  if (!p) return { ok: false, reason: "Party not found" };
  if (p.inviteToken !== token) return { ok: false, reason: "Invalid invite token" };
  return { ok: true, party: p };
}

export function addGuestRSVP(partyId, { name, photos }) {
  const p = getParty(partyId);
  if (!p) throw new Error("Party not found");

  const guest = {
    id: uuidv4(),
    name,
    numPhotos: photos.length,
    time: new Date().toISOString(),
    photoNames: photos.map((f) => f.name),
  };

  p.guests.unshift(guest);
  p.logs.unshift({
    id: uuidv4(),
    time: new Date().toISOString(),
    result: "RSVP_RECEIVED",
    name: guest.name,
  });

  save(state);
  return guest;
}

export function addLog(partyId, log) {
  const p = getParty(partyId);
  if (!p) throw new Error("Party not found");

  p.logs.unshift({
    id: uuidv4(),
    time: new Date().toISOString(),
    ...log,
  });

  save(state);
}