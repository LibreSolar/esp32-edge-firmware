<template>
  <v-container fill-height fluid>
    <v-layout text-center align-center>
      <v-flex>
        <v-card max-width=600 class="mx-auto my-auto">
          <v-img :src="require('../assets/logo.png')" contain height="200"></v-img>
          <v-card-title primary-title class="justify-center">Welcome!</v-card-title>
          <v-card-text>
            <div class="ma-auto">
              Select device from above
            </div>
          </v-card-text>
        </v-card>
      </v-flex>
    </v-layout>
  </v-container>
</template>

<script>
export default {}
</script>
