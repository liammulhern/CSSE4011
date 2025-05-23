<script setup lang="ts">
import { ref } from 'vue';
import { useRouter, useRoute } from 'vue-router';
import { useForm } from 'vee-validate';
import { toTypedSchema } from '@vee-validate/zod';
import { login } from '@/services/auth';
import { useAuthStore } from '@/stores/auth';
import * as z from 'zod';
import { Card, CardHeader, CardTitle, CardContent, CardFooter } from '@/components/ui/card';
import { Input } from '@/components/ui/input';
import { Button } from '@/components/ui/button';
import { Checkbox } from '@/components/ui/checkbox'
import { Label } from '@/components/ui/label'
import { FormControl, FormField, FormLabel, FormItem } from '@/components/ui/form';

const formSchema = toTypedSchema(z.object({
  password: z.string(),
  email: z.string(),
}));

const form = useForm({
  validationSchema: formSchema,
});

const loginError = ref<string | null>(null);
const router = useRouter();
const route = useRoute();

const onSubmit = form.handleSubmit(async (values: LoginValues) => {
  loginError.value = null;

  try {
    await login(values.email, values.password);

    const auth = useAuthStore();
    await auth.fetchUser();

    const redirect = route.query.redirect || '/dashboard';

    router.push(redirect);

  } catch (err: any) {
    loginError.value = 'Login failed';
  }
});

</script>

<template>
  <main class="h-screen w-screen flex items-center justify-center">
    <Card class="max-w-[320px] md:max-w-[400px] w-full">
      <CardHeader>
        <CardTitle class="text-center">Login</CardTitle>
      </CardHeader>
      <CardContent>
        <form @submit.prevent="onSubmit">
          <FormField v-slot="{ componentField }" name="email">
            <FormItem class="mb-4">
              <FormLabel>Email</FormLabel>
              <FormControl>
                <Input type="text" placeholder="john_smith@mail.com" v-bind="componentField" required />
              </FormControl>
              <FormMessage />
            </FormItem>
          </FormField>
          <FormField v-slot="{ componentField }" name="password">
            <FormItem>
              <FormLabel>Password</FormLabel>
              <FormControl>
                <Input type="password" v-bind="componentField" required />
              </FormControl>
              <FormMessage />
            </FormItem>
          </FormField>
          <p v-if="loginError" class="text-red-500 mt-2">{{ loginError }}</p>
          <div class="w-full mt-5">
            <Button type="submit" class="w-full" @click="onSubmit">Login</Button>
          </div>
        </form>
      </CardContent>
    </Card>
  </main>
</template>
