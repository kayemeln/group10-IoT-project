/* eslint-disable */
// this is an auto generated file. This will be overwritten

export const createParty = /* GraphQL */ `
  mutation CreateParty(
    $input: CreatePartyInput!
    $condition: ModelPartyConditionInput
  ) {
    createParty(input: $input, condition: $condition) {
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
export const updateParty = /* GraphQL */ `
  mutation UpdateParty(
    $input: UpdatePartyInput!
    $condition: ModelPartyConditionInput
  ) {
    updateParty(input: $input, condition: $condition) {
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
export const deleteParty = /* GraphQL */ `
  mutation DeleteParty(
    $input: DeletePartyInput!
    $condition: ModelPartyConditionInput
  ) {
    deleteParty(input: $input, condition: $condition) {
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
export const createGuestUpload = /* GraphQL */ `
  mutation CreateGuestUpload(
    $input: CreateGuestUploadInput!
    $condition: ModelGuestUploadConditionInput
  ) {
    createGuestUpload(input: $input, condition: $condition) {
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
export const updateGuestUpload = /* GraphQL */ `
  mutation UpdateGuestUpload(
    $input: UpdateGuestUploadInput!
    $condition: ModelGuestUploadConditionInput
  ) {
    updateGuestUpload(input: $input, condition: $condition) {
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
export const deleteGuestUpload = /* GraphQL */ `
  mutation DeleteGuestUpload(
    $input: DeleteGuestUploadInput!
    $condition: ModelGuestUploadConditionInput
  ) {
    deleteGuestUpload(input: $input, condition: $condition) {
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
