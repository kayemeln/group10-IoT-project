/* eslint-disable */
// this is an auto generated file. This will be overwritten

export const onCreateParty = /* GraphQL */ `
  subscription OnCreateParty($filter: ModelSubscriptionPartyFilterInput) {
    onCreateParty(filter: $filter) {
      id
      partyName
      hostEmail
      inviteToken
      password
      uploads {
        nextToken
        __typename
      }
      createdAt
      updatedAt
      __typename
    }
  }
`;
export const onUpdateParty = /* GraphQL */ `
  subscription OnUpdateParty($filter: ModelSubscriptionPartyFilterInput) {
    onUpdateParty(filter: $filter) {
      id
      partyName
      hostEmail
      inviteToken
      password
      uploads {
        nextToken
        __typename
      }
      createdAt
      updatedAt
      __typename
    }
  }
`;
export const onDeleteParty = /* GraphQL */ `
  subscription OnDeleteParty($filter: ModelSubscriptionPartyFilterInput) {
    onDeleteParty(filter: $filter) {
      id
      partyName
      hostEmail
      inviteToken
      password
      uploads {
        nextToken
        __typename
      }
      createdAt
      updatedAt
      __typename
    }
  }
`;
export const onCreateGuestUpload = /* GraphQL */ `
  subscription OnCreateGuestUpload(
    $filter: ModelSubscriptionGuestUploadFilterInput
  ) {
    onCreateGuestUpload(filter: $filter) {
      id
      partyId
      guestName
      photoPaths
      uploadedAt
      createdAt
      updatedAt
      __typename
    }
  }
`;
export const onUpdateGuestUpload = /* GraphQL */ `
  subscription OnUpdateGuestUpload(
    $filter: ModelSubscriptionGuestUploadFilterInput
  ) {
    onUpdateGuestUpload(filter: $filter) {
      id
      partyId
      guestName
      photoPaths
      uploadedAt
      createdAt
      updatedAt
      __typename
    }
  }
`;
export const onDeleteGuestUpload = /* GraphQL */ `
  subscription OnDeleteGuestUpload(
    $filter: ModelSubscriptionGuestUploadFilterInput
  ) {
    onDeleteGuestUpload(filter: $filter) {
      id
      partyId
      guestName
      photoPaths
      uploadedAt
      createdAt
      updatedAt
      __typename
    }
  }
`;
