import React, { useEffect, useState } from "react";
import { useSearchParams } from "react-router-dom";
import { generateClient } from "aws-amplify/api";
import { uploadData } from "aws-amplify/storage";

import { getParty } from "../graphql/queries";
import { createGuestUpload } from "../graphql/mutations";

//helper function so images get uploaded with guest name
function slugify(text) {
  return String(text)
    .toLowerCase()
    .trim()
    .replace(/['"]/g, "")
    .replace(/[^a-z0-9]+/g, "-")
    .replace(/^-+|-+$/g, "");
}

export default function GuestUpload() {
  const client = generateClient();
  const [params] = useSearchParams();
  const partyId = params.get("party");
  const token = params.get("token");

  const [party, setParty] = useState(null);
  const [name, setName] = useState("");
  const [files, setFiles] = useState([]);
  const [error, setError] = useState("");
  const [done, setDone] = useState(false);

  // Load and validate party
  useEffect(() => {
    async function loadParty() {
      try {
        const result = await client.graphql({
          query: getParty,
          variables: { id: partyId },
        });

        const foundParty = result.data.getParty;

        if (!foundParty) {
          setError("Party not found");
          return;
        }

        if (foundParty.inviteToken !== token) {
          setError("Invalid invite link");
          return;
        }

        setParty(foundParty);
      } catch (err) {
        console.error("LOAD PARTY ERROR:", err);
        setError("Could not load party");
      }
    }

    if (partyId && token) {
      loadParty();
    }
  }, [partyId, token]);

  // Upload handler
  const handleSubmit = async (e) => {
    e.preventDefault();
    setError("");

    if (!party) return;
    if (!name.trim()) return setError("Enter your name");
    if (files.length === 0) return setError("Choose at least one image");

    try {
      const uploadedPaths = [];

for (let i = 0; i < files.length; i++) {
  const file = files[i];

  const partySlug = `${slugify(party.partyName)}-${partyId.slice(0, 8)}`;
  const guestSlug = slugify(name.trim());

  const extension = file.name.includes(".")
    ? file.name.split(".").pop().toLowerCase()
    : "jpg";

  const path = `public/parties/${partySlug}/${guestSlug}/${Date.now()}-${guestSlug}-${i + 1}.${extension}`;

  try {
    const result = await uploadData({
      path,
      data: file,
      options: {
        contentType: file.type || "image/jpeg",
      },
    }).result;

    console.log("UPLOAD SUCCESS:", result);
    uploadedPaths.push(path);
  } catch (err) {
    console.error("UPLOAD ERROR:", err);
    throw err;
  }
}

      // Save metadata to GraphQL
      await client.graphql({
        query: createGuestUpload,
        variables: {
          input: {
            partyId,
            guestName: name.trim(),
            photoPaths: uploadedPaths,
            uploadedAt: new Date().toISOString(),
          },
        },
      });

      setDone(true);
    } catch (err) {
      console.error("FINAL ERROR:", err);
      setError("Upload failed - check console");
    }
  };

  // UI states
  if (error) return <div style={{ padding: 20 }}>{error}</div>;
  if (!party) return <div style={{ padding: 20 }}>Loading...</div>;
  if (done) return <div style={{ padding: 20 }}>Upload complete ✅</div>;

  return (
    <div style={{ padding: 20 }}>
      <h2>{party.partyName}</h2>

      <form onSubmit={handleSubmit}>
        <input
          value={name}
          onChange={(e) => setName(e.target.value)}
          placeholder="Your name"
        />

        <br /><br />

        <input
          type="file"
          accept="image/*"
          multiple
          onChange={(e) => setFiles(Array.from(e.target.files || []))}
        />

        <br /><br />

        <button type="submit">Upload</button>
      </form>
    </div>
  );
}