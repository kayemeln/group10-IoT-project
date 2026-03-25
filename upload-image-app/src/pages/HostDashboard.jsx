import React, { useEffect, useState } from "react";
import { useParams, Link } from "react-router-dom";
import { generateClient } from "aws-amplify/api";
import { getUrl } from "aws-amplify/storage";

import { getParty, guestUploadsByParty } from "../graphql/queries";


function fmt(iso) {
  try {
    return new Date(iso).toLocaleString();
  } catch {
    return iso;
  }
}

export default function HostDashboard() {
  const client = generateClient();
  const { partyId } = useParams();
  const [party, setParty] = useState(null);
  const [uploads, setUploads] = useState([]);
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    async function loadData() {
      setError("");

      try {
        const partyResult = await client.graphql({
          query: getParty,
          variables: { id: partyId },
        });

        const foundParty = partyResult?.data?.getParty;

        if (!foundParty) {
          setError("Party not found");
          setLoading(false);
          return;
        }

        const uploadsResult = await client.graphql({
          query: guestUploadsByParty,
          variables: {
            partyId,
            sortDirection: "DESC",
          },
        });

        const rawUploads = uploadsResult?.data?.guestUploadsByParty?.items || [];

        const uploadsWithUrls = await Promise.all(
          rawUploads.map(async (upload) => {
            const photoUrls = await Promise.all(
              (upload.photoPaths || []).map(async (path) => {
                try {
                  const signed = await getUrl({ path });
                  return {
                    path,
                    url: signed.url.toString(),
                  };
                } catch (err) {
                  console.error("GET URL ERROR:", err);
                  return {
                    path,
                    url: null,
                  };
                }
              })
            );

            return {
              ...upload,
              photoUrls,
            };
          })
        );

        setParty(foundParty);
        setUploads(uploadsWithUrls);
      } catch (err) {
        console.error("DASHBOARD LOAD ERROR:", err);
        setError("Failed to load dashboard");
      } finally {
        setLoading(false);
      }
    }

    if (partyId) {
      loadData();
    }
  }, [partyId]);

  if (loading) {
    return <div style={{ padding: 20 }}>Loading...</div>;
  }

  if (error) {
    return (
      <div style={{ padding: 20 }}>
        <h2>Host Dashboard</h2>
        <p>{error}</p>
        <Link to="/create-party">Back</Link>
      </div>
    );
  }

  const inviteUrl = `${window.location.origin}/guest?party=${party.id}&token=${party.inviteToken}`;

  return (
    <div style={{ padding: 20 }}>
      <h2>Host Dashboard</h2>

      <div style={{ marginBottom: 24 }}>
        <h3>{party.partyName}</h3>
        <p>Host: {party.hostEmail}</p>
        <input readOnly value={inviteUrl} style={{ width: "100%", maxWidth: 700 }} />
      </div>

      <h3>Guest Uploads</h3>

      {uploads.length === 0 ? (
        <p>No uploads yet.</p>
      ) : (
        uploads.map((upload) => (
          <div
            key={upload.id}
            style={{
              marginBottom: 24,
              padding: 16,
              border: "1px solid #ddd",
              borderRadius: 8,
            }}
          >
            <p>
              <strong>{upload.guestName}</strong>{" "}
              <span style={{ color: "#666" }}>
                ({upload.photoPaths?.length || 0} photos)
              </span>
            </p>

            <p style={{ color: "#666" }}>{fmt(upload.uploadedAt)}</p>

            <div
              style={{
                display: "grid",
                gridTemplateColumns: "repeat(auto-fill, 120px)",
                gap: 10,
                marginTop: 10,
              }}
            >
              {upload.photoUrls.map((photo, index) => (
                <div key={photo.path || index}>
                  {photo.url ? (
                    <img
                      src={photo.url}
                      alt={`${upload.guestName}-${index + 1}`}
                      style={{
                        width: 120,
                        height: 120,
                        objectFit: "cover",
                        borderRadius: 8,
                        display: "block",
                      }}
                    />
                  ) : (
                    <div
                      style={{
                        width: 120,
                        height: 120,
                        borderRadius: 8,
                        border: "1px solid #ddd",
                        display: "flex",
                        alignItems: "center",
                        justifyContent: "center",
                        color: "#666",
                        fontSize: 12,
                        textAlign: "center",
                        padding: 8,
                        background: "#f8f8f8",
                      }}
                    >
                      Could not load image
                    </div>
                  )}
                </div>
              ))}
            </div>
          </div>
        ))
      )}
    </div>
  );
}